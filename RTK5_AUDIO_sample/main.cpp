//=====================================================================//
/*! @file
    @brief  RX65N オーディオファイル再生サンプル
    @author 平松邦仁 (hira@rvf-rc45.net)
	@copyright	Copyright (C) 2018 Kunihito Hiramatsu @n
				Released under the MIT license @n
				https://github.com/hirakuni45/RX/blob/master/LICENSE
*/
//=====================================================================//
#include "common/renesas.hpp"
#include "common/cmt_io.hpp"
#include "common/fixed_fifo.hpp"
#include "common/sci_io.hpp"
#include "common/format.hpp"
#include "common/command.hpp"
#include "common/spi_io2.hpp"
#include "common/sdc_man.hpp"
#include "common/tpu_io.hpp"
#include "common/audio_out.hpp"
#include "common/mp3_in.hpp"

namespace {

	typedef device::PORT<device::PORT7, device::bitpos::B0> LED;
//	typedef device::PORT<device::PORT0, device::bitpos::B5> SW2;

	typedef device::system_io<12000000> SYSTEM_IO;

	device::cmt_io<device::CMT0, utils::null_task>  cmt_;

	typedef utils::fixed_fifo<char, 512>  RECV_BUFF;
	typedef utils::fixed_fifo<char, 1024> SEND_BUFF;
	typedef device::sci_io<device::SCI9, RECV_BUFF, SEND_BUFF> SCI;
	SCI		sci_;

	typedef device::PORT<device::PORT6, device::bitpos::B3> LCD_DISP;
	typedef device::PORT<device::PORT6, device::bitpos::B6> LCD_LIGHT;
	typedef device::glcdc_io<device::GLCDC, 480, 272, device::PIX_TYPE::RGB565> GLCDC_IO;
	GLCDC_IO	glcdc_io_;

	// カード電源制御は使わないので、「device::NULL_PORT」を指定する。
//	typedef device::PORT<device::PORT6, device::bitpos::B4> SDC_POWER;
	typedef device::NULL_PORT SDC_POWER;

#ifdef SDHI_IF
	typedef fatfs::sdhi_io<device::SDHI, SDC_POWER> SDHI;
	SDHI	sdh_;
#else
	// Soft SDC 用　SPI 定義（SPI）
	typedef device::PORT<device::PORT2, device::bitpos::B2> MISO;  // DAT0
	typedef device::PORT<device::PORT2, device::bitpos::B0> MOSI;  // CMD
	typedef device::PORT<device::PORT2, device::bitpos::B1> SPCK;  // CLK

	typedef device::spi_io2<MISO, MOSI, SPCK> SPI;  ///< Soft SPI 定義

	SPI		spi_;

	typedef device::PORT<device::PORT1, device::bitpos::B7> SDC_SELECT;  // DAT3 カード選択信号
	typedef device::PORT<device::PORT2, device::bitpos::B5> SDC_DETECT;  // CD   カード検出

	typedef fatfs::mmc_io<SPI, SDC_SELECT, SDC_POWER, SDC_DETECT> MMC;   // ハードウェアー定義

	MMC		sdh_(spi_, 20000000);
#endif
	typedef utils::sdc_man SDC;
	SDC		sdc_;

	utils::command<256> cmd_;

	volatile uint32_t	wpos_;

	/// DMAC 終了割り込み
	class dmac_term_task {
	public:
		void operator() () {
			device::DMAC0::DMCNT.DTE = 1;  // DMA を再スタート
			wpos_ = 0;
		}
	};

	typedef device::dmac_mgr<device::DMAC0, dmac_term_task> DMAC_MGR;
	DMAC_MGR	dmac_mgr_;

	uint32_t get_wave_pos_() { return (dmac_mgr_.get_count() & 0x3ff) ^ 0x3ff; }

	typedef device::R12DA DAC;
	typedef device::dac_out<DAC> DAC_OUT;
	DAC_OUT		dac_out_;

	typedef utils::audio_out<8192, 1024> AUDIO_OUT;
	AUDIO_OUT	audio_out_;

	class tpu_task {
	public:
		void operator() () {
			uint32_t tmp = wpos_;
			++wpos_;
			if((tmp ^ wpos_) & 64) {
				audio_out_.service(64);
			}
		}
	};

	typedef device::tpu_io<device::TPU0, tpu_task> TPU0;
	TPU0		tpu0_;

	audio::mp3_in	mp3_in_;

	bool check_mount_() {
		auto f = sdc_.get_mount();
		if(!f) {
			utils::format("SD card not mount.\n");
		}
		return f;
	}


	bool play_mp3_(const char* fname)
	{
		utils::file_io fin;
		if(!fin.open(fname, "rb")) {
			return false;
		}
		bool ret = mp3_in_.decode(fin, audio_out_);
		fin.close();
		return ret;
	}


	void play_loop_(const char*);


	void play_loop_func_(const char* name, const FILINFO* fi, bool dir, void* option)
	{
		if(dir) {
			play_loop_(name);
		} else {
			const char* ext = strrchr(name, '.');
			if(ext != nullptr) {
				if(strcmp(ext, ".wav") == 0) {
///					play_wav_(name);
				} else if(strcmp(ext, ".mp3") == 0) {
					play_mp3_(name);
				}
			}
		}
	}


	void play_loop_(const char* root)
	{
		sdc_.set_dir_list_limit(1);
		sdc_.start_dir_list(root, play_loop_func_);
	}


	void command_()
	{
		if(!cmd_.service()) {
			return;
		}

		uint8_t cmdn = cmd_.get_words();
		if(cmdn >= 1) {
			bool f = false;
			if(cmd_.cmp_word(0, "dir")) {  // dir [xxx]
				if(check_mount_()) {
					if(cmdn >= 2) {
						char tmp[128];
						cmd_.get_word(1, sizeof(tmp), tmp);
						sdc_.dir(tmp);
					} else {
						sdc_.dir("");
					}
				}
				f = true;
			} else if(cmd_.cmp_word(0, "cd")) {  // cd [xxx]
				if(check_mount_()) {
					if(cmdn >= 2) {
						char tmp[128];
						cmd_.get_word(1, sizeof(tmp), tmp);
						sdc_.cd(tmp);						
					} else {
						sdc_.cd("/");
					}
				}
				f = true;
			} else if(cmd_.cmp_word(0, "pwd")) { // pwd
				utils::format("%s\n") % sdc_.get_current();
				f = true;
			} else if(cmd_.cmp_word(0, "play")) {
				if(cmdn >= 2) {
					char tmp[128];
					cmd_.get_word(1, sizeof(tmp), tmp);
					if(std::strcmp(tmp, "*") == 0) {
						play_loop_("");
					} else {
						play_mp3_(tmp);
					}
				} else {
					play_loop_("");
				}
				f = true;
			} else if(cmd_.cmp_word(0, "help")) {
				utils::format("    dir [path]\n");
				utils::format("    cd [path]\n");
				utils::format("    pwd\n");
				f = true;
			}
			if(!f) {
				char tmp[128];
				if(cmd_.get_word(0, sizeof(tmp), tmp)) {
					utils::format("Command error: '%s'\n") % tmp;
				}
			}
		}
	}
}

extern "C" {

	void set_sample_rate(uint32_t freq)
	{
		uint8_t intr_level = 5;
		if(!tpu0_.start(freq, intr_level)) {
			utils::format("TPU0 start error...\n");
		}
	}


	void sci_putch(char ch)
	{
		sci_.putch(ch);
	}


	void sci_puts(const char* str)
	{
		sci_.puts(str);
	}


	char sci_getch(void)
	{
		return sci_.getch();
	}


	uint16_t sci_length()
	{
		return sci_.recv_length();
	}


	DSTATUS disk_initialize(BYTE drv) {
		return sdh_.disk_initialize(drv);
	}


	DSTATUS disk_status(BYTE drv) {
		return sdh_.disk_status(drv);
	}


	DRESULT disk_read(BYTE drv, BYTE* buff, DWORD sector, UINT count) {
		return sdh_.disk_read(drv, buff, sector, count);
	}


	DRESULT disk_write(BYTE drv, const BYTE* buff, DWORD sector, UINT count) {
		return sdh_.disk_write(drv, buff, sector, count);
	}


	DRESULT disk_ioctl(BYTE drv, BYTE ctrl, void* buff) {
		return sdh_.disk_ioctl(drv, ctrl, buff);
	}


	DWORD get_fattime(void) {
		time_t t = 0;
///		rtc_.get_time(t);
		return utils::str::get_fattime(t);
	}


	void utf8_to_sjis(const char* src, char* dst, uint32_t len) {
		utils::str::utf8_to_sjis(src, dst, len);
	}


	int fatfs_get_mount() {
		return check_mount_();
	}
}

int main(int argc, char** argv);

int main(int argc, char** argv)
{
	SYSTEM_IO::setup_system_clock();

	{  // タイマー設定（６０Ｈｚ）
		uint8_t cmt_irq_level = 4;
		cmt_.start(60, cmt_irq_level);
	}

	{  // SCI 設定
		static const uint8_t sci_level = 2;
		sci_.start(115200, sci_level);
	}

	{  // 内臓１２ビット D/A の設定
		bool amp_ena = true;
		dac_out_.start(DAC_OUT::output::CH0_CH1, amp_ena);
		dac_out_.out0(0x8000);
		dac_out_.out1(0x8000);
	}

	{  // SD カード・クラスの初期化
		sdh_.start();
		sdc_.start();
	}

	// 波形メモリーの無音状態初期化
	audio_out_.mute();

	{  // サンプリング・タイマー設定
		set_sample_rate(44100);
	}

	{  // DMAC マネージャー開始
		uint8_t intr_level = 4;
		auto ret = dmac_mgr_.start(tpu0_.get_intr_vec(), DMAC_MGR::trans_type::SP_DN_32,
			reinterpret_cast<uint32_t>(audio_out_.get_wave()), DAC::DADR0.address(),
			audio_out_.size(), intr_level);
		if(!ret) {
			utils::format("DMAC Not start...\n");
		}
	}

	utils::format("RTK5RX65N Start for AUDIO sample\n");
	cmd_.set_prompt("# ");

	{  // GLCDC 初期化
		LCD_DISP::DIR  = 1;
		LCD_LIGHT::DIR = 1;
		LCD_DISP::P  = 1;  // DISP Enable
		LCD_LIGHT::P = 1;  // BackLight Enable (No PWM)
		if(glcdc_io_.start()) {
			utils::format("Start GLCDC\n");
		} else {
			utils::format("Fail GLCDC\n");
		}
	}

#if 0
	device::GLCDC::GR1CLUT0[0].R = 0xcc;
	device::GLCDC::GR1CLUT0[1] = 0x123456;
	uint32_t rgba = device::GLCDC::GR1CLUT0[2];
	utils::format("%08X\n") % rgba;
#endif

	LED::DIR = 1;

	uint8_t n = 0;
	while(1) {
		cmt_.sync();

		sdc_.service(sdh_.service());

		command_();

		++n;
		if(n >= 30) {
			n = 0;
		}
		if(n < 10) {
			LED::P = 0;
		} else {
			LED::P = 1;
		}
	}
}
