#pragma once
//=========================================================================//
/*! @file
    @brief  TCP Protocol 全体管理 @n
			Copyright 2017 Kunihito Hiramatsu
    @author 平松邦仁 (hira@rvf-rc45.net)
*/
//=========================================================================//
#include "net2/tcp.hpp"

namespace net {

	//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++//
	/*!
		@brief  TCP マネージ・クラス
		@param[in]	ETHER	イーサーネット・ドライバー・クラス
		@param[in]	NMAX	管理最大数
	*/
	//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++//
	template<class ETHER, uint32_t NMAX>
	class tcp_manage {

		ETHER&		eth_;

		tcp			tcp_[NMAX];

	public:
		//-----------------------------------------------------------------//
		/*!
			@brief  コンストラクター
			@param[in]	eth		イーサーネット・ドライバー
		*/
		//-----------------------------------------------------------------//
		tcp_manage(ETHER& eth) : eth_(eth)
		{ }


		//-----------------------------------------------------------------//
		/*!
			@brief  格納可能な最大サイズを返す
			@return 格納可能な最大サイズ
		*/
		//-----------------------------------------------------------------//
		uint32_t capacity() const noexcept { return NMAX; }


		//-----------------------------------------------------------------//
		/*!
			@brief  パース
			@param[in]	eh	イーサーネット・ヘッダー
			@param[in]	ih	IPV4 ヘッダー
			@param[in]	msg	メッセージ先頭
			@param[in]	len	メッセージ長
			@return エラーが無い場合「true」
		*/
		//-----------------------------------------------------------------//
		bool parse(const eth_h& eh, const ipv4_h& ih, const void* msg, int32_t len)
		{
			return true;
		}


		//-----------------------------------------------------------------//
		/*!
			@brief  オープン
			@param[in]	adrs	アドレス
			@param[in]	port	ポート
			@return ディスクリプタ
		*/
		//-----------------------------------------------------------------//
		int open(const ip_adrs& adrs, uint16_t port)
		{
			int ds = -1;


			return ds;
		}


		//-----------------------------------------------------------------//
		/*!
			@brief  サービス（１０ｍｓ毎に呼ぶ）
		*/
		//-----------------------------------------------------------------//
		void service()
		{
			for(uint32_t i = 0; i < NMAX; ++i) {
				tcp_[i].service();
			}
		}
	};
}
