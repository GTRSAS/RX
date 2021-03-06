#pragma once
//=========================================================================//
/*!	@file
	@brief	RX グループ・SCI I/O 制御 @n
			・DMAC による転送をサポートしていませんが、必要性を感じていません。@n
			・同期通信で、ブロック転送を行うような場合は、必要かもしれません。@n
			Ex: 定義例 @n
			・受信バッファ、送信バッファの大きさは、最低１６バイトは必要でしょう。@n
			・ボーレート、サービスする内容に応じて適切に設定して下さい。@n
			  typedef utils::fixed_fifo<char, 512>  RECV_BUFF;  // 受信バッファ定義 @n
			  typedef utils::fixed_fifo<char, 1024> SEND_BUFF;  // 送信バッファ定義 @n
			  typedef device::sci_io<device::SCI1, RECV_BUFF, SEND_BUFF> SCI;  // SCI1 の場合 @n
			  SCI	sci_; // 実態の宣言 @n
			Ex: 開始例 @n
			  uint8_t intr_level = 2;          // 割り込みレベル(2) @n
			  sci_.start(115200, intr_level);  // ボーレート設定(115200) @n
			Ex: POSIX 関数 (printf など) への通路設定 @n
			  extern "C" { @n
				void sci_putch(char ch) @n
				{ @n
					sci_.putch(ch); @n
				} @n
				char sci_getch(void) @n
				{ @n
					return sci_.getch(); @n
				} @n
			  }; @n
			// 上記関数を定義しておけば、syscalls.c との連携で、printf が使えるようになる。@n
			// ※ C++ では printf は推奨しないし使う理由が無い、utils::format を使って下さい。
    @author 平松邦仁 (hira@rvf-rc45.net)
	@copyright	Copyright (C) 2013, 2018 Kunihito Hiramatsu @n
				Released under the MIT license @n
				https://github.com/hirakuni45/RX/blob/master/LICENSE
*/
//=========================================================================//
#include "common/renesas.hpp"
#include "common/vect.h"

/// F_PCLKB はボーレートパラメーター計算に必要で、設定が無いとエラーにします。
#ifndef F_PCLKB
#  error "sci_io.hpp requires F_PCLKB to be defined"
#endif

namespace device {

	//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++//
	/*!
		@brief  SCI I/O 制御クラス
		@param[in]	SCI		SCI 型
		@param[in]	RBF		受信バッファクラス
		@param[in]	SBF		送信バッファクラス
		@param[in]	PSEL	ポート選択
	*/
	//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++//
	template <class SCI, class RBF, class SBF, port_map::option PSEL = port_map::option::FIRST>
	class sci_io {
	public:
		//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++//
		/*!
			@brief	SCI 通信プロトコル型
		*/
		//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++//
		enum class PROTOCOL {
			B8_N_1S,	///< 8 ビット、No-Parity、 1 Stop Bit
			B8_E_1S,	///< 8 ビット、Even(偶数)、1 Stop Bit
			B8_O_1S,	///< 8 ビット、Odd (奇数)、1 Stop Bit
			B8_N_2S,	///< 8 ビット、No-Parity、 2 Stop Bits
			B8_E_2S,	///< 8 ビット、Even(偶数)、2 Stop Bits
			B8_O_2S,	///< 8 ビット、Odd (奇数)、2 Stop Bits
		};

	private:
		static RBF	recv_;
		static SBF	send_;
#if defined(SIG_RX64M) || defined(SIG_RX71M) || defined(SIG_RX65N)
		static volatile bool send_stall_;
#endif
		uint8_t		level_;
		bool		auto_crlf_;

		// ※マルチタスクの場合適切な実装をする
		void sleep_() noexcept { asm("nop"); }


		static INTERRUPT_FUNC void recv_task_()
		{
			bool err = false;
			if(SCI::SSR.ORER()) {	///< 受信オーバランエラー状態確認
				SCI::SSR.ORER = 0;	///< 受信オーバランエラークリア
				err = true;
			}
			///< フレーミングエラー/パリティエラー状態確認
			if(SCI::SSR() & (SCI::SSR.FER.b() | SCI::SSR.PER.b())) {
				// エラーフラグの消去
				SCI::SSR.FER = 0;
				SCI::SSR.PER = 0;
				err = true;
			}
			volatile uint8_t data = SCI::RDR();
			if(!err) {
				recv_.put(data);
			}
		}


		static INTERRUPT_FUNC void send_task_()
		{
#if defined(SIG_RX64M) || defined(SIG_RX71M) || defined(SIG_RX65N)
			if(send_.length() > 0) {
				SCI::TDR = send_.get();
			}
			if(send_.length() == 0) {
				SCI::SCR.TIE = 0;
				send_stall_ = true;
			}
#else
			SCI::TDR = send_.get();
			if(send_.length() == 0) {
				SCI::SCR.TEIE = 0;
			}
#endif
		}


		void set_vector_(ICU::VECTOR rx_vec, ICU::VECTOR tx_vec) noexcept
		{
			if(level_) {
				set_interrupt_task(recv_task_, static_cast<uint32_t>(rx_vec));
				set_interrupt_task(send_task_, static_cast<uint32_t>(tx_vec));
			} else {
				set_interrupt_task(nullptr, static_cast<uint32_t>(rx_vec));
				set_interrupt_task(nullptr, static_cast<uint32_t>(tx_vec));
			}
		}

		void set_intr_() noexcept
		{
#if defined(SIG_RX64M) || defined(SIG_RX71M) || defined(SIG_RX65N)
			set_vector_(SCI::get_rx_vec(), SCI::get_tx_vec());
#else
			set_vector_(SCI::get_rx_vec(), SCI::get_te_vec());
#endif
			icu_mgr::set_level(SCI::get_peripheral(), level_);
		}

	public:
		//-----------------------------------------------------------------//
		/*!
			@brief  コンストラクター
			@param[in]	autocrlf	LF 時、自動で CR の送出をしない場合「false」
		*/
		//-----------------------------------------------------------------//
		sci_io(bool autocrlf = true) noexcept : level_(0), auto_crlf_(autocrlf) { }


		//-----------------------------------------------------------------//
		/*!
			@brief  ボーレートを設定して、SCI を有効にする @n
					※RX63T では、ポーリングはサポート外
			@param[in]	baud	ボーレート
			@param[in]	level	割り込みレベル（０の場合ポーリング）
			@param[in]	prot	通信プロトコル（標準は、８ビット、パリティ無し、１ストップ）
			@return エラーなら「false」
		*/
		//-----------------------------------------------------------------//
		bool start(uint32_t baud, uint8_t level = 0, PROTOCOL prot = PROTOCOL::B8_N_1S) noexcept
		{
#if defined(SIG_RX64M) || defined(SIG_RX71M) || defined(SIG_RX65N)
			send_stall_ = true;
#endif
#if defined(SIG_RX63T)
			if(level == 0) return false;
#endif
			level_ = level;

			power_cfg::turn(SCI::get_peripheral());

			SCI::SCR = 0x00;			// TE, RE disable.

			port_map::turn(SCI::get_peripheral(), true, PSEL);

			uint32_t brr = F_PCLKB / baud * 16;
			uint8_t cks = 0;
			while(brr > (512 << 8)) {
				brr >>= 2;
				++cks;
			}
			if(cks > 3) return false;
			bool abcs = true;
			if(brr > (256 << 8)) { brr /= 2; abcs = false; }
			uint32_t mddr = ((brr & 0xff00) << 8) / brr;
			brr >>= 8;

			set_intr_();

			bool stop = 0;
			bool pm = 0;
			bool pe = 0;
			switch(prot) {
			case PROTOCOL::B8_N_1S:
				stop = 0;
				pm = 0;
				pe = 0;
				break;
			case PROTOCOL::B8_E_1S:
				stop = 0;
				pm = 0;
				pe = 1;
				break;
			case PROTOCOL::B8_O_1S:
				stop = 0;
				pm = 1;
				pe = 1;
				break;
			case PROTOCOL::B8_N_2S:
				stop = 1;
				pm = 0;
				pe = 0;
				break;
			case PROTOCOL::B8_E_2S:
				stop = 1;
				pm = 0;
				pe = 1;
				break;
			case PROTOCOL::B8_O_2S:
				stop = 1;
				pm = 1;
				pe = 1;
				break;
			default:
				return false;
			}
			SCI::SMR = SCI::SMR.CKS.b(cks) | SCI::SMR.STOP.b(stop)
					 | SCI::SMR.PM.b(pm) | SCI::SMR.PE.b(pe);
			bool brme = false;
			if(mddr >= 128) brme = true;
			SCI::SEMR = SCI::SEMR.ABCS.b(abcs) | SCI::SEMR.BRME.b(brme);
			if(brr) --brr;
			SCI::BRR = brr;
			SCI::MDDR = mddr;

			if(level > 0) {
				SCI::SCR = SCI::SCR.RIE.b() | SCI::SCR.TE.b() | SCI::SCR.RE.b();
			} else {
				SCI::SCR = SCI::SCR.TE.b() | SCI::SCR.RE.b();
			}

			return true;
		}


		//-----------------------------------------------------------------//
		/*!
			@brief	LF 時、CR 自動送出
			@param[in]	f	「false」なら無効
		 */
		//-----------------------------------------------------------------//
		void auto_crlf(bool f = true) noexcept { auto_crlf_ = f; }


		//-----------------------------------------------------------------//
		/*!
			@brief	SCI 出力バッファのサイズを返す
			@return　バッファのサイズ
		 */
		//-----------------------------------------------------------------//
		uint32_t send_length() const noexcept
		{
			if(level_) {
				return send_.length();
			} else {
				return 0;
			}
		}


		//-----------------------------------------------------------------//
		/*!
			@brief	SCI 文字出力
			@param[in]	ch	文字コード
		 */
		//-----------------------------------------------------------------//
		void putch(char ch)  noexcept
		{
			if(auto_crlf_ && ch == '\n') {
				putch('\r');
			}

			if(level_) {
				volatile bool b = SCI::SSR.ORER();
				if(b) {
					SCI::SSR.ORER = 0;
				}
				/// 送信バッファの容量が７／８以上の場合は、空になるまで待つ。
				if(send_.length() >= (send_.size() * 7 / 8)) {
					while(send_.length() != 0) sleep_();
				}
				send_.put(ch);
#if defined(SIG_RX64M) || defined(SIG_RX71M) || defined(SIG_RX65N)
				SCI::SCR.TIE = 0;
				if(send_stall_) {
					while(SCI::SSR.TEND() == 0) sleep_();
					SCI::TDR = send_.get();
					if(send_.length() > 0) {
						send_stall_ = false;
					}
				}
				SCI::SCR.TIE = !send_stall_;
#else
				if(SCI::SCR.TEIE() == 0) {
					SCI::SCR.TEIE = 1;
				}
#endif
			} else {
				while(SCI::SSR.TEND() == 0) sleep_();
				SCI::TDR = ch;
			}
		}


		//-----------------------------------------------------------------//
		/*!
			@brief	SCI 入力文字数を取得
			@return	入力文字数
		 */
		//-----------------------------------------------------------------//
		uint32_t recv_length()  noexcept
		{
			if(level_) {
				return recv_.length();
			} else {
				if(SCI::SSR.ORER()) {	///< 受信オーバランエラー状態確認
					SCI::SSR.ORER = 0;	///< 受信オーバランエラークリア
				}
#if defined(SIG_RX64M) || defined(SIG_RX71M) || defined(SIG_RX24T) || defined(SIG_RX65N)
				return SCI::SSR.RDRF();
#else
				return 0;
#endif
			}
		}


		//-----------------------------------------------------------------//
		/*!
			@brief	SCI 文字入力（ブロック関数）
			@return 文字コード
		 */
		//-----------------------------------------------------------------//
		char getch() noexcept
		{
			if(level_) {
				while(recv_.length() == 0) sleep_();
				return recv_.get();
			} else {
				while(recv_length() == 0) sleep_();
				return SCI::RDR();	///< 受信データ読み出し
			}
		}


		//-----------------------------------------------------------------//
		/*!
			@brief	文字列出力
			@param[in]	s	出力文字列
		 */
		//-----------------------------------------------------------------//
		void puts(const char* s)  noexcept
		{
			if(s == nullptr) return;
			char ch;
			while((ch = *s++) != 0) {
				putch(ch);
			}
		}
	};

	// テンプレート関数、実態の定義
	template<class SCI, class RBF, class SBF, port_map::option PSEL>
		RBF sci_io<SCI, RBF, SBF, PSEL>::recv_;
	template<class SCI, class RBF, class SBF, port_map::option PSEL>
		SBF sci_io<SCI, RBF, SBF, PSEL>::send_;
#if defined(SIG_RX64M) || defined(SIG_RX71M) || defined(SIG_RX65N)
	template<class SCI, class RBF, class SBF, port_map::option PSEL>
		volatile bool sci_io<SCI, RBF, SBF, PSEL>::send_stall_;
#endif
}
