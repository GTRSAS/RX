#pragma once
//=====================================================================//
/*!	@file
	@brief	RX24T グループ・割り込みマネージャー
    @author 平松邦仁 (hira@rvf-rc45.net)
	@copyright	Copyright (C) 2016,2017 Kunihito Hiramatsu @n
				Released under the MIT license @n
				https://github.com/hirakuni45/RX/blob/master/LICENSE
*/
//=====================================================================//
#include "RX24T/icu.hpp"
#include "RX24T/peripheral.hpp"

namespace device {

	//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++//
	/*!
		@brief  割り込みマネージャー・クラス
	*/
	//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++//
	struct icu_mgr {

		//-----------------------------------------------------------------//
		/*!
			@brief  割り込みを設定する（ベクター別）
			@param[in]	vec	割り込みベクター
			@param[in]	lvl	割り込みレベル（０の場合、割り込み禁止）
			@return 正常なら「true」
		*/
		//-----------------------------------------------------------------//
		static bool set_level(ICU::VECTOR vec, uint8_t lvl)
		{
			bool ret = true;
			bool ena = lvl != 0 ? true : false;
			switch(vec) {

			case ICU::VECTOR::TGIA0:
				ICU::IPR.MTU0_ABCD = lvl;
				ICU::IER.TGIA0 = ena;
				break;
			case ICU::VECTOR::TGIB0:
				ICU::IPR.MTU0_ABCD = lvl;
				ICU::IER.TGIB0 = ena;
				break;
			case ICU::VECTOR::TGIC0:
				ICU::IPR.MTU0_ABCD = lvl;
				ICU::IER.TGIC0 = ena;
				break;
			case ICU::VECTOR::TGID0:
				ICU::IPR.MTU0_ABCD = lvl;
				ICU::IER.TGID0 = ena;
				break;
			case ICU::VECTOR::TCIV0:
				ICU::IPR.MTU0_VEF = lvl;
				ICU::IER.TCIV0 = ena;
				break;
			case ICU::VECTOR::TGIE0:
				ICU::IPR.MTU0_VEF = lvl;
				ICU::IER.TGIE0 = ena;
				break;
			case ICU::VECTOR::TGIF0:
				ICU::IPR.MTU0_VEF = lvl;
				ICU::IER.TGIF0 = ena;
				break;

			case ICU::VECTOR::TGIA1:
				ICU::IPR.MTU1_AB = lvl;
				ICU::IER.TGIA1 = ena;
				break;
			case ICU::VECTOR::TGIB1:
				ICU::IPR.MTU1_AB = lvl;
				ICU::IER.TGIB1 = ena;
				break;
			case ICU::VECTOR::TCIV1:
				ICU::IPR.MTU1_VU = lvl;
				ICU::IER.TCIV1 = ena;
				break;
			case ICU::VECTOR::TCIU1:
				ICU::IPR.MTU1_VU = lvl;
				ICU::IER.TCIU1 = ena;
				break;

			case ICU::VECTOR::TGIA2:
				ICU::IPR.MTU2_AB = lvl;
				ICU::IER.TGIA2 = ena;
				break;
			case ICU::VECTOR::TGIB2:
				ICU::IPR.MTU2_AB = lvl;
				ICU::IER.TGIB2 = ena;
				break;
			case ICU::VECTOR::TCIV2:
				ICU::IPR.MTU2_VU = lvl;
				ICU::IER.TCIV2 = ena;
				break;
			case ICU::VECTOR::TCIU2:
				ICU::IPR.MTU2_VU = lvl;
				ICU::IER.TCIU2 = ena;
				break;

			case ICU::VECTOR::TGIA3:
				ICU::IPR.MTU3_ABCD = lvl;
				ICU::IER.TGIA3 = ena;
				break;
			case ICU::VECTOR::TGIB3:
				ICU::IPR.MTU3_ABCD = lvl;
				ICU::IER.TGIB3 = ena;
				break;
			case ICU::VECTOR::TGIC3:
				ICU::IPR.MTU3_ABCD = lvl;
				ICU::IER.TGIC3 = ena;
				break;
			case ICU::VECTOR::TGID3:
				ICU::IPR.MTU3_ABCD = lvl;
				ICU::IER.TGID3 = ena;
				break;
			case ICU::VECTOR::TCIV3:
				ICU::IPR.MTU3_V = lvl;
				ICU::IER.TCIV3 = ena;
				break;

			case ICU::VECTOR::TGIA4:
				ICU::IPR.MTU4_ABCD = lvl;
				ICU::IER.TGIA4 = ena;
				break;
			case ICU::VECTOR::TGIB4:
				ICU::IPR.MTU4_ABCD = lvl;
				ICU::IER.TGIB4 = ena;
				break;
			case ICU::VECTOR::TGIC4:
				ICU::IPR.MTU4_ABCD = lvl;
				ICU::IER.TGIC4 = ena;
				break;
			case ICU::VECTOR::TGID4:
				ICU::IPR.MTU4_ABCD = lvl;
				ICU::IER.TGID4 = ena;
				break;
			case ICU::VECTOR::TCIV4:
				ICU::IPR.MTU4_V = lvl;
				ICU::IER.TCIV4 = ena;
				break;

			case ICU::VECTOR::TGIU5:
				ICU::IPR.MTU5_UVW = lvl;
				ICU::IER.TGIU5 = ena;
				break;
			case ICU::VECTOR::TGIV5:
				ICU::IPR.MTU5_UVW = lvl;
				ICU::IER.TGIV5 = ena;
				break;
			case ICU::VECTOR::TGIW5:
				ICU::IPR.MTU5_UVW = lvl;
				ICU::IER.TGIW5 = ena;
				break;

			case ICU::VECTOR::TGIA6:
				ICU::IPR.MTU6_ABCD = lvl;
				ICU::IER.TGIA6 = ena;
				break;
			case ICU::VECTOR::TGIB6:
				ICU::IPR.MTU6_ABCD = lvl;
				ICU::IER.TGIB6 = ena;
				break;
			case ICU::VECTOR::TGIC6:
				ICU::IPR.MTU6_ABCD = lvl;
				ICU::IER.TGIC6 = ena;
				break;
			case ICU::VECTOR::TGID6:
				ICU::IPR.MTU6_ABCD = lvl;
				ICU::IER.TGID6 = ena;
				break;
			case ICU::VECTOR::TCIV6:
				ICU::IPR.MTU6_V = lvl;
				ICU::IER.TCIV6 = ena;
				break;

			case ICU::VECTOR::TGIA7:
				ICU::IPR.MTU7_AB = lvl;
				ICU::IER.TGIA7 = ena;
				break;
			case ICU::VECTOR::TGIB7:
				ICU::IPR.MTU7_AB = lvl;
				ICU::IER.TGIB7 = ena;
				break;
			case ICU::VECTOR::TGIC7:
				ICU::IPR.MTU7_CD = lvl;
				ICU::IER.TGIC7 = ena;
				break;
			case ICU::VECTOR::TGID7:
				ICU::IPR.MTU7_CD = lvl;
				ICU::IER.TGID7 = ena;
				break;
			case ICU::VECTOR::TCIV7:
				ICU::IPR.MTU7_V = lvl;
				ICU::IER.TCIV7 = ena;
				break;

			case ICU::VECTOR::TGIA9:
				ICU::IPR.MTU9_ABCD = lvl;
				ICU::IER.TGIA9 = ena;
				break;
			case ICU::VECTOR::TGIB9:
				ICU::IPR.MTU9_ABCD = lvl;
				ICU::IER.TGIB9 = ena;
				break;
			case ICU::VECTOR::TGIC9:
				ICU::IPR.MTU9_ABCD = lvl;
				ICU::IER.TGIC9 = ena;
				break;
			case ICU::VECTOR::TGID9:
				ICU::IPR.MTU9_ABCD = lvl;
				ICU::IER.TGID9 = ena;
				break;
			case ICU::VECTOR::TCIV9:
				ICU::IPR.MTU9_VEF = lvl;
				ICU::IER.TCIV9 = ena;
				break;
			case ICU::VECTOR::TGIE9:
				ICU::IPR.MTU9_VEF = lvl;
				ICU::IER.TGIE9 = ena;
				break;
			case ICU::VECTOR::TGIF9:
				ICU::IPR.MTU9_VEF = lvl;
				ICU::IER.TGIF9 = ena;
				break;

			default:
				ret = false;
				break;
			}
			return ret;
		}


		//-----------------------------------------------------------------//
		/*!
			@brief  割り込みを設定する
			@param[in]	t	周辺機器タイプ
			@param[in]	lvl	割り込みレベル（０の場合、割り込み禁止）
			@return 正常なら「true」
		*/
		//-----------------------------------------------------------------//
		static bool set_level(peripheral t, uint8_t lvl)
		{
			bool ret = true;
			bool ena = lvl != 0 ? true : false;
			switch(t) {
			case peripheral::CMT0:
				ICU::IPR.CMI0 = lvl;
				ICU::IER.CMI0 = ena;
				break;

			case peripheral::CMT1:
				ICU::IPR.CMI1 = lvl;
				ICU::IER.CMI1 = ena;
				break;

			case peripheral::CMT2:
				ICU::IPR.CMI2 = lvl;
				ICU::IER.CMI2 = ena;
				break;

			case peripheral::CMT3:
				ICU::IPR.CMI3 = lvl;
				ICU::IER.CMI3 = ena;
				break;

			case peripheral::RSPI0:
				ICU::IPR.RSPI0 = lvl;
				ICU::IER.SPRI0 = ena;
				ICU::IER.SPTI0 = ena;
				break;

			case peripheral::SCI1:
			case peripheral::SCI1C:
				ICU::IPR.SCI1 = lvl;
				ICU::IER.RXI1 = ena;
				ICU::IER.TEI1 = ena;
				break;

			case peripheral::SCI5:
			case peripheral::SCI5C:
				ICU::IPR.SCI5 = lvl;
				ICU::IER.RXI5 = ena;
				ICU::IER.TEI5 = ena;
				break;

			case peripheral::SCI6:
			case peripheral::SCI6C:
				ICU::IPR.SCI6 = lvl;
				ICU::IER.RXI6 = ena;
				ICU::IER.TEI6 = ena;
				break;

			case peripheral::RIIC0:
				ICU::IPR.EEI0 = lvl;
				ICU::IPR.RXI0 = lvl;
				ICU::IPR.TXI0 = lvl;
				ICU::IPR.TEI0 = lvl;
				ICU::IER.EEI0 = ena;
				ICU::IER.RXI0 = ena;
				ICU::IER.TXI0 = ena;
				ICU::IER.TEI0 = ena;
				break;

			case peripheral::S12AD:
				ICU::IPR.S12ADI = lvl;
				ICU::IPR.GBADI  = lvl;
				ICU::IPR.GCADI  = lvl;
				ICU::IER.S12ADI = ena;
				ICU::IER.GBADI  = ena;
				ICU::IER.GCADI  = ena;
				break;

			case peripheral::S12AD1:
				ICU::IPR.S12ADI1 = lvl;
				ICU::IPR.GBADI1  = lvl;
				ICU::IPR.GCADI1  = lvl;
				ICU::IER.S12ADI1 = ena;
				ICU::IER.GBADI1  = ena;
				ICU::IER.GCADI1  = ena;
				break;

			case peripheral::S12AD2:
				ICU::IPR.S12ADI2 = lvl;
				ICU::IPR.GBADI2  = lvl;
				ICU::IPR.GCADI2  = lvl;
				ICU::IER.S12ADI2 = ena;
				ICU::IER.GBADI2  = ena;
				ICU::IER.GCADI2  = ena;
				break;

			default:
				ret = false;
				break;
			}
			return ret;
		}
	};
}
