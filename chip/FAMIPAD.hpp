#pragma once
//=====================================================================//
/*!	@file
	@brief	ファミリーコンピューター JOY PAD 読み込み @n
			CMOS 4021B Shift Register
    @author 平松邦仁 (hira@rvf-rc45.net)
	@copyright	Copyright (C) 2018 Kunihito Hiramatsu @n
				Released under the MIT license @n
				https://github.com/hirakuni45/RX/blob/master/LICENSE
*/
//=====================================================================//
#include "common/delay.hpp"

namespace chip {

	//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++//
	/*!
		@brief  PAD 情報型
	*/
	//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++//
	enum class FAMIPAD_ST : uint8_t {
		RIGHT  = 0b00000001,
		LEFT   = 0b00000010,
		DOWN   = 0b00000100,
		UP     = 0b00001000,
		START  = 0b00010000,
		SELECT = 0b00100000,
		B      = 0b01000000,
		A      = 0b10000000,
	};


	//---------------------------------------------------------------------//
	/*!
		@brief	状態を検査
		@param[in]	data	パッドの状態
		@param[in]	pad		検査パッド
	 */
	//---------------------------------------------------------------------//
	static inline bool on(uint8_t data, FAMIPAD_ST pad) noexcept
	{
		return data & static_cast<uint8_t>(pad);
	}


	//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++//
	/*!
		@brief  FAMIPAD テンプレートクラス
		@param[in]	P_S		P/S ポート
		@param[in]	CLK		CLK ポート
		@param[in]	OUT		OUT ポート
	*/
	//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++//
	template <class P_S, class CLK, class OUT>
	class FAMIPAD {

		uint8_t		data_;

	public:
		//-----------------------------------------------------------------//
		/*!
			@brief	コンストラクタ
		 */
		//-----------------------------------------------------------------//
		FAMIPAD() noexcept : data_(0) { }


		//-----------------------------------------------------------------//
		/*!
			@brief	開始
			@param[in]	pullup	「OUT」信号入力をプルアップする場合
		 */
		//-----------------------------------------------------------------//
		void start(bool pullup = false) noexcept
		{
			P_S::DIR = 1;
			CLK::DIR = 1;
			OUT::DIR = 0;
			if(pullup) OUT::PU = 1;
			update();
		}


		//-----------------------------------------------------------------//
		/*!
			@brief	最新の状態に更新
			@return 状態データ
		 */
		//-----------------------------------------------------------------//
		uint8_t update() noexcept
		{
			P_S::P = 0; // seirial
			uint8_t d = 0;
			for(uint8_t i = 0; i < 8; ++i) {
				d <<= 1;
				if(!OUT::P()) ++d;
				CLK::P = 1;
				utils::delay::loop(6);  // 多分「250ns」くらい
				CLK::P = 0;
				utils::delay::loop(6);
			}
			P_S::P = 1; // parallel
			data_ = d;
			return data_;
		}


		//-----------------------------------------------------------------//
		/*!
			@brief	状態を検査
			@param[in]	pad		検査パッド
		 */
		//-----------------------------------------------------------------//
		bool on(FAMIPAD_ST pad) const noexcept
		{
			return data_ & static_cast<uint8_t>(pad);
		}


		//-----------------------------------------------------------------//
		/*!
			@brief	[] オペレーター
			@param[in]	pad		検査パッド
		 */
		//-----------------------------------------------------------------//
		bool operator [] (FAMIPAD_ST pad) const noexcept
		{
			return on(pad);
		}
	};
}
