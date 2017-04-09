#pragma once
//=====================================================================//
/*! @file
    @brief  SEEDA03 (RX64M) コア @n
			Copyright 2017 Kunihito Hiramatsu
    @author 平松邦仁 (hira@rvf-rc45.net)
*/
//=====================================================================//
#include <string>
#include "common/renesas.hpp"

#include "common/cmt_io.hpp"
#include "common/fifo.hpp"
#include "common/sci_io.hpp"
#include "common/format.hpp"
#include "common/input.hpp"
#include "common/command.hpp"
#include "common/rspi_io.hpp"
#include "common/spi_io.hpp"
#include "common/sdc_io.hpp"

#include "chip/LTC2348_16.hpp"

// #define SERVER_TASK

#ifdef SERVER_TASK
#include <cstdlib>
#include "GR/core/Ethernet.h"
#endif

namespace seeda {

	//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++//
	/*!
		@brief  コア・クラス
	*/
	//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++//
	struct core {

		static const int seeda_version_ = 15;
		static const uint32_t build_id_ = B_ID;

		class cmt_task {
			volatile unsigned long millis_;
			volatile unsigned long delay_;
			volatile uint32_t millis10x_;
			volatile uint32_t cmtdiv_;

		public:
			cmt_task() : millis_(0), delay_(0), millis10x_(0), cmtdiv_(0) { }

			void operator() () {
				++millis_;
				++cmtdiv_;
				if(cmtdiv_ >= 10) {
					cmtdiv_ = 0;
					++millis10x_;
				}
				if(delay_) {
					--delay_;
				}
			}

			void sync_100hz()
			{
				volatile uint32_t tmp = millis10x_;
				while(tmp == millis10x_) ;
			}

			volatile unsigned long get_millis() const { return millis_; }
			volatile unsigned long get_delay() const { return delay_; }
			void set_delay(volatile unsigned long n) { delay_ = n; }
		};

		typedef device::cmt_io<device::CMT0, cmt_task> CMT0;
		CMT0	cmt0_;

//		typedef device::cmt_io<device::CMT1, null_task> CMT1;

		typedef utils::fifo<uint8_t, 1024> BUFFER;
		typedef device::sci_io<device::SCI5, BUFFER, BUFFER> SCI;
		SCI		sci_;

		typedef utils::rtc_io RTC;
		RTC		rtc_;

		typedef device::PORT<device::PORT6, device::bitpos::B7> SW1;
		typedef device::PORT<device::PORT6, device::bitpos::B6> SW2;

		typedef device::PORT<device::PORT7, device::bitpos::B0> LAN_RESN;
		typedef device::PORT<device::PORT7, device::bitpos::B3> LAN_PDN;

		// Soft SDC 用　SPI 定義（SPI）
		typedef device::PORT<device::PORTD, device::bitpos::B6> MISO;
		typedef device::PORT<device::PORTD, device::bitpos::B4> MOSI;
		typedef device::PORT<device::PORTD, device::bitpos::B5> SPCK;
		typedef device::spi_io<MISO, MOSI, SPCK> SPI;
		SPI		spi_;

		typedef device::PORT<device::PORTD, device::bitpos::B3> sdc_select;	///< カード選択信号
		typedef device::NULL_PORT  sdc_power;	///< カード電源制御（常に電源ＯＮ）
		typedef device::PORT<device::PORTE, device::bitpos::B6> sdc_detect;	///< カード検出

		typedef utils::sdc_io<SPI, sdc_select, sdc_power, sdc_detect> SDC;
		SDC		sdc_;

		// LTC A/D 定義
		typedef device::PORT<device::PORT4, device::bitpos::B0> LTC_CSN;   // P40(141)
		typedef device::PORT<device::PORTC, device::bitpos::B6> LTC_CNV;   // PC6(61)
		typedef device::PORT<device::PORTD, device::bitpos::B0> LTC_BUSY;  // PD0/IRQ0(126)
		typedef device::PORT<device::PORT5, device::bitpos::B3> LTC_PD;    // P53(53)
		typedef device::PORT<device::PORT5, device::bitpos::B6> LTC_SDI;   // P56(50)
		typedef device::PORT<device::PORT8, device::bitpos::B6> LTC_SCKO;  // P86(41)
		typedef device::PORT<device::PORT8, device::bitpos::B7> LTC_SCKI;  // P87(39)
		typedef device::PORT<device::PORT2, device::bitpos::B0> LTC_SDO0;  // P20(37)
		typedef device::PORT<device::PORT2, device::bitpos::B1> LTC_SDO2;  // P21(36)
		typedef device::PORT<device::PORT2, device::bitpos::B2> LTC_SDO4;  // P22(35)
		typedef device::PORT<device::PORT2, device::bitpos::B3> LTC_SDO6;  // P23(34)
		typedef struct chip::LTC2348_SDO_t<LTC_SCKO, LTC_SDO0, LTC_SDO2, LTC_SDO4, LTC_SDO6> LTC_SDO;
		typedef chip::LTC2348_16<LTC_CSN, LTC_CNV, LTC_BUSY, LTC_PD, LTC_SDI, LTC_SCKI, LTC_SDO> EADC;
		EADC	eadc_;

		utils::command<256> cmd_;

#ifdef SERVER_TASK
		EthernetServer server_;
#endif

		uint8_t get_switch_()
		{
			return static_cast<uint8_t>(!SW1::P()) | (static_cast<uint8_t>(!SW2::P()) << 1);
		}

		time_t get_time_()
		{
			time_t t = 0;
			if(!rtc_.get_time(t)) {
				utils::format("Stall RTC read\n");
			}
			return t;
		}

		void disp_time_(time_t t, char* dst = nullptr, uint32_t size = 0)
		{
			struct tm *m = localtime(&t);
			utils::format("%s %s %d %02d:%02d:%02d  %4d\n", dst, size)
				% get_wday(m->tm_wday)
				% get_mon(m->tm_mon)
				% static_cast<uint32_t>(m->tm_mday)
				% static_cast<uint32_t>(m->tm_hour)
				% static_cast<uint32_t>(m->tm_min)
				% static_cast<uint32_t>(m->tm_sec)
				% static_cast<uint32_t>(m->tm_year + 1900);
		}

	const char* get_dec_(const char* p, char tmch, int& value) {
		int v = 0;
		char ch;
		while((ch = *p) != 0) {
			++p;
			if(ch == tmch) {
				break;
			} else if(ch >= '0' && ch <= '9') {
				v *= 10;
				v += ch - '0';
			} else {
				return nullptr;
			}
		}
		value = v;
		return p;
	}

	void set_time_date_()
	{
		time_t t = get_time_();
		if(t == 0) return;

		struct tm *m = localtime(&t);
		bool err = false;
		if(cmd_.get_words() == 3) {
			char buff[12];
			if(cmd_.get_word(1, sizeof(buff), buff)) {
				const char* p = buff;
				int vs[3];
				uint8_t i;
				for(i = 0; i < 3; ++i) {
					p = get_dec_(p, '/', vs[i]);
					if(p == nullptr) {
						break;
					}
				}
				if(p != nullptr && p[0] == 0 && i == 3) {
					if(vs[0] >= 1900 && vs[0] < 2100) m->tm_year = vs[0] - 1900;
					if(vs[1] >= 1 && vs[1] <= 12) m->tm_mon = vs[1] - 1;
					if(vs[2] >= 1 && vs[2] <= 31) m->tm_mday = vs[2];		
				} else {
					err = true;
				}
			}

			if(cmd_.get_word(2, sizeof(buff), buff)) {
				const char* p = buff;
				int vs[3];
				uint8_t i;
				for(i = 0; i < 3; ++i) {
					p = get_dec_(p, ':', vs[i]);
					if(p == nullptr) {
						break;
					}
				}
				if(p != nullptr && p[0] == 0 && (i == 2 || i == 3)) {
					if(vs[0] >= 0 && vs[0] < 24) m->tm_hour = vs[0];
					if(vs[1] >= 0 && vs[1] < 60) m->tm_min = vs[1];
					if(i == 3 && vs[2] >= 0 && vs[2] < 60) m->tm_sec = vs[2];
					else m->tm_sec = 0;
				} else {
					err = true;
				}
			}
		}

		if(err) {
			sci_puts("Can't analize Time/Date input.\n");
			return;
		}

		time_t tt = mktime(m);
		if(!rtc_.set_time(tt)) {
			sci_puts("Stall RTC write...\n");
		}
	}


	bool check_mount_() {
		auto f = sdc_.get_mount();
		if(!f) {
			utils::format("SD card not mount.\n");
		}
		return f;
	}


	bool reset_signal_(uint8_t cmdn)
	{
		bool f = false;
		if(cmdn == 1) {
			bool v = LAN_RESN::P();
			utils::format("LAN-RESN: %d\n") % static_cast<int>(v);
			return true;
		} else if(cmdn > 1) {
			char tmp[16];
			if(cmd_.get_word(1, sizeof(tmp), tmp)) {
				// Reset signal
				if(strcmp(tmp, "0") == 0) {
					device::PORT7::PODR.B0 = 0;
					f = true;
				} else if(strcmp(tmp, "1") == 0) {
					device::PORT7::PODR.B0 = 1;
					f = true;
				} else {
					utils::format("reset param error: '%s'\n") % tmp;
				}
			}
		}
		return f;
	}


	bool eadc_conv_(uint8_t cmdn)
	{
		bool f = false;
		if(cmdn == 1) {
			if(eadc_.convert()) {
				for(int i = 0; i < 8; ++i) {
					uint32_t v = eadc_.get_data(i);
					float fv = eadc_.get_voltage(i);
					utils::format("LTC2348-16(%d): CHID: %d, SPAN: %03b, %6.3f [V] (%d)\n")
						% i % ((v >> 3) & 7) % (v & 7) % fv % (v >> 8);
				}
			} else {
				utils::format("LTC2348-16 convert error\n");
			}
			return true;
		} else if(cmdn > 1) {
			char tmp[16];
			if(cmd_.get_word(1, sizeof(tmp), tmp)) {
				if(tmp[0] >= '0' && tmp[0] <= '7' && tmp[1] == 0) {
					if(eadc_.convert()) {
						int ch = tmp[0] - '0';
						uint32_t v = eadc_.get_value(ch);
						float fv = eadc_.get_voltage(ch);
						utils::format("LTC2348-16(%d): %6.3f [V] (%d)\n") % ch % fv % v;
					} else {
						utils::format("LTC2348-16 convert error\n");
					}
				} else {
					utils::format("convert param error: '%s'\n") % tmp;
				}
				f = true;
			}
		}
		return f;
	}


	bool eadc_span_(uint8_t cmdn)
	{
		if(cmdn >= 3) {

		} else {
			
		}
		return true;
	}


	bool eadc_sample_(uint8_t cmdn)
	{
		if(cmdn < 8) {
			utils::format("sample param error...\n"); 
		} else {
			int mode = 0;
			int ch = -1;
			int rate = -1;
			int num = -1;
			char fname[32];
			fname[0] = 0;
			for(uint8_t i = 1; i < cmdn; ++i) {
				char tmp[32];
				if(cmd_.get_word(i, sizeof(tmp), tmp)) {
					if(tmp[0] == '-') {
						if(strcmp(tmp, "-ch") == 0) {
							mode = 1;
						} else if(strcmp(tmp, "-rate") == 0) {
							mode = 2;
						} else if(strcmp(tmp, "-num") == 0) {
							mode = 3;
						} else {
							utils::format("sample option error: '%s'\n") % tmp;
							return true;
						}
					} else {
						bool f = false;
						switch(mode) {
						case 0:
							strcpy(fname, tmp);
							f = true;
							break;
						case 1:
							f = (utils::input("%d", tmp) % ch).status();
							break;
						case 2:
							f = (utils::input("%d", tmp) % rate).status();
							break;
						case 3:
							f = (utils::input("%d", tmp) % num).status();
							break;
						}
						mode = 0;
						if(!f) {
							utils::format("sample option value error: '%s'\n") % tmp;
							return true;
						}
					}
				} else {
					return false;
				}
			}
			if(ch > 7 || ch < 0) {
				utils::format("sample chanel out range: %d\n") % ch;
			}

			utils::format("ch:   %d\n") % ch;
			utils::format("rate: %d\n") % rate;
			utils::format("num:  %d\n") % num;
			utils::format("name: '%s'\n") % fname;
		}
		return true;
	}

	public:
		//-----------------------------------------------------------------//
		/*!
			@brief  コンストラクター
		*/
		//-----------------------------------------------------------------//
		core() : sdc_(spi_, 10000000)
#ifdef SERVER_TASK
			,server_(80)
#endif
		{ }


		//-----------------------------------------------------------------//
		/*!
			@brief  初期化
		*/
		//-----------------------------------------------------------------//
		void init()
		{
			{  // DIP-SW プルアップ
				SW1::DIR = 0;  // input
				SW2::DIR = 0;  // input
				SW1::PU = 1;
				SW2::PU = 1;
			}

			{  // タイマー設定、１０００Ｈｚ（１ｍｓ）
				uint8_t int_level = 1;
				cmt0_.start(1000, int_level);
			}

			{  // SCI 設定
				uint8_t int_level = 2;
				sci_.start(115200, int_level);
			}

			{  // RTC 設定
				rtc_.start();
			}

			{  // SD カード・クラスの初期化
				sdc_.start();
			}

			{  // LTC2348ILX-16 初期化
				// 内臓リファレンスと内臓バッファ
				// VREFIN: 2.024V、VREFBUF: 4.096V、Analog range: 0V to 5.12V
				if(!eadc_.start(1000000, EADC::span_type::P5_12)) {
					utils::format("LTC2348_16 not found...\n");
				}
			}

			{  // LAN initialize (PHY reset, PHY POWER-DOWN
				LAN_PDN::DIR = 1;  // output;
				LAN_PDN::P = 1;    // Not Power Down Mode..
				LAN_RESN::DIR = 1; // output;

				LAN_RESN::P = 0;
				utils::delay::milli_second(200); /// reset time
				LAN_RESN::P = 1;
			}

			cmd_.set_prompt("# ");
		}


		//-----------------------------------------------------------------//
		/*!
			@brief  初期化
		*/
		//-----------------------------------------------------------------//
		void title()
		{
			// タイトル・コール
			utils::format("\nStart Seeda03 Build:%u Version %d.%02d\n") % build_id_
				% (seeda_version_ / 100) % (seeda_version_ % 100);
			utils::format("Endian: %3b") % static_cast<uint32_t>(device::SYSTEM::MDE.MDE());
			utils::format(", PCKA: %u [Hz]") % static_cast<uint32_t>(F_PCKA);
			utils::format(", PCKB: %u [Hz]\n") % static_cast<uint32_t>(F_PCKB);
			utils::format("DIP-Switch: %d\n") % static_cast<int>(get_switch_());
			if(eadc_.probe()) {
				utils::format("Device LTC2348-16: Ready\n");
			} else {
				utils::format("Device LTC2348-16: Not Ready\n");
			}
			{
				time_t t = get_time_();
				if(t != 0) {
					disp_time_(t);
				}
			}
		}


		//-----------------------------------------------------------------//
		/*!
			@brief  サービス
		*/
		//-----------------------------------------------------------------//
		void service()
		{
			cmt0_.at_task().sync_100hz();

#ifdef SERVER_TASK
			Ethernet.mainloop();

			service_server();
#endif

			sdc_.service();

			// コマンド入力と、コマンド解析
			if(cmd_.service()) {
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
					} else if(cmd_.cmp_word(0, "date")) {
						if(cmdn == 1) {
							time_t t = get_time_();
							if(t != 0) {
								disp_time_(t);
							}
						} else {
							set_time_date_();
						}
						f = true;
					} else if(cmd_.cmp_word(0, "reset")) {
						f = reset_signal_(cmdn);
					} else if(cmd_.cmp_word(0, "eadc")) {
						f = eadc_conv_(cmdn);
					} else if(cmd_.cmp_word(0, "span")) {
						f = eadc_span_(cmdn);
					} else if(cmd_.cmp_word(0, "sample")) {
						f = eadc_sample_(cmdn);
					} else if(cmd_.cmp_word(0, "help") || cmd_.cmp_word(0, "?")) {
						utils::format("date\n");
						utils::format("date yyyy/mm/dd hh:mm[:ss]\n");
						utils::format("dir [name]\n");
						utils::format("cd [directory-name]\n");
						utils::format("pwd\n");
						utils::format("reset [01]  (PHY reset signal)\n");
						utils::format("eadc [0-7]  (LTC2348 A/D conversion)\n");
						utils::format("span CH(0-7) SPAN(0-7)  (LTC2348 A/D span setting)\n"); 
						utils::format("sample -ch 0-7 -rate FRQ -num SAMPLE-NUM file-name (LTC2348 A/D sample)\n");
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
	};
}