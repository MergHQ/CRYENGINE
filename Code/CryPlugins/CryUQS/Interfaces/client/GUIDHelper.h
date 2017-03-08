// Copyright 2001-2016 Crytek GmbH / Crytek Group. All rights reserved.

#pragma once

// *INDENT-OFF* - <hard to read code and declarations due to inconsistent indentation>

namespace uqs
{
	namespace client
	{
		namespace internal
		{

			//===================================================================================
			//
			// CGUIDHelper
			//
			// - intermediate helper class to extend CryGUID's functionality until Timur submits his changes
			//
			//===================================================================================

			// example GUID as string: "dbbb8931-6ab6-42a7-b67e-3e871f7d9bdd"
			//                          u32      u16  u16  2*u8 6*u8
			//                          012345678901234567890123456789012345
			//                                    1         2         3

			class CGUIDHelper
			{
			public:

				static CryGUID  FromString(const char* szGuidAsString);
				static void     ToString(shared::IUqsString& out, const CryGUID& guid);

			private:

				static uint8    HexCharToUint8(char hexChar);
				static uint16   HexCharToUint16(char hexChar);
				static uint32   HexCharToUint32(char hexChar);
			};

			inline CryGUID CGUIDHelper::FromString(const char* szGuidAsString)
			{
				// example GUID as string: "dbbb8931-6ab6-42a7-b67e-3e871f7d9bdd"
				//                          u32      u16  u16  2*u8 6*u8
				//                          012345678901234567890123456789012345
				//                                    1         2         3

				assert(strlen(szGuidAsString) == 36 && szGuidAsString[8] == '-' && szGuidAsString[13] == '-' && szGuidAsString[18] == '-' && szGuidAsString[23] == '-');

				uint32 data1;
				uint16 data2;
				uint16 data3;
				uint8 data4[8];

				data1 =
					(HexCharToUint32(szGuidAsString[0]) << 28) |
					(HexCharToUint32(szGuidAsString[1]) << 24) |
					(HexCharToUint32(szGuidAsString[2]) << 20) |
					(HexCharToUint32(szGuidAsString[3]) << 16) |
					(HexCharToUint32(szGuidAsString[4]) << 12) |
					(HexCharToUint32(szGuidAsString[5]) << 8) |
					(HexCharToUint32(szGuidAsString[6]) << 4) |
					(HexCharToUint32(szGuidAsString[7]) << 0);

				// [8] == '-' (separator)

				data2 =
					(HexCharToUint16(szGuidAsString[9]) << 12) |
					(HexCharToUint16(szGuidAsString[10]) << 8) |
					(HexCharToUint16(szGuidAsString[11]) << 4) |
					(HexCharToUint16(szGuidAsString[12]) << 0);

				// [13] == '-' (separator)

				data3 =
					(HexCharToUint16(szGuidAsString[14]) << 12) |
					(HexCharToUint16(szGuidAsString[15]) << 8) |
					(HexCharToUint16(szGuidAsString[16]) << 4) |
					(HexCharToUint16(szGuidAsString[17]) << 0);

				data4[0] =
					(HexCharToUint8(szGuidAsString[19]) << 4) |
					(HexCharToUint8(szGuidAsString[20]) << 0);

				data4[1] =
					(HexCharToUint8(szGuidAsString[21]) << 4) |
					(HexCharToUint8(szGuidAsString[22]) << 0);

				// [23] == '-' (separator)

				data4[2] =
					(HexCharToUint8(szGuidAsString[24]) << 4) |
					(HexCharToUint8(szGuidAsString[25]) << 0);

				data4[3] =
					(HexCharToUint8(szGuidAsString[26]) << 4) |
					(HexCharToUint8(szGuidAsString[27]) << 0);

				data4[4] =
					(HexCharToUint8(szGuidAsString[28]) << 4) |
					(HexCharToUint8(szGuidAsString[29]) << 0);

				data4[5] =
					(HexCharToUint8(szGuidAsString[30]) << 4) |
					(HexCharToUint8(szGuidAsString[31]) << 0);

				data4[6] =
					(HexCharToUint8(szGuidAsString[32]) << 4) |
					(HexCharToUint8(szGuidAsString[33]) << 0);

				data4[7] =
					(HexCharToUint8(szGuidAsString[34]) << 4) |
					(HexCharToUint8(szGuidAsString[35]) << 0);

				return CryGUID::Construct(data1, data2, data3, data4[0], data4[1], data4[2], data4[3], data4[4], data4[5], data4[6], data4[7]);
			}

			inline void CGUIDHelper::ToString(shared::IUqsString& out, const CryGUID& guid)
			{
				uint32 data1;
				uint16 data2;
				uint16 data3;
				uint8 data4[8];

				// example GUID as string: "dbbb8931-6ab6-42a7-b67e-3e871f7d9bdd"
				//                          u32      u16  u16  2*u8 6*u8

				//
				// high part
				//

				data1 = (uint32)guid.hipart;
				data2 = (uint16)(guid.hipart >> 32);
				data3 = (uint16)(guid.hipart >> 48);

				//
				// low part
				//

				data4[0] = (uint8)guid.lopart;
				data4[1] = (uint8)(guid.lopart >> 8);
				data4[2] = (uint8)(guid.lopart >> 16);
				data4[3] = (uint8)(guid.lopart >> 24);
				data4[4] = (uint8)(guid.lopart >> 32);
				data4[5] = (uint8)(guid.lopart >> 40);
				data4[6] = (uint8)(guid.lopart >> 48);
				data4[7] = (uint8)(guid.lopart >> 56);

				static_assert(sizeof(unsigned int) >= sizeof(uint32), "'unsinged int' is expected to be >= uint32 for the format string to work");

				out.Format("%08x-%04x-%04x-%02x%02x-%02x%02x%02x%02x%02x%02x",
					(unsigned int)data1,
					(unsigned int)data2,
					(unsigned int)data3,
					(unsigned int)data4[0],
					(unsigned int)data4[1],
					(unsigned int)data4[2],
					(unsigned int)data4[3],
					(unsigned int)data4[4],
					(unsigned int)data4[5],
					(unsigned int)data4[6],
					(unsigned int)data4[7]);
			}

			inline uint8 CGUIDHelper::HexCharToUint8(char hexChar)
			{
				if (hexChar >= '0' && hexChar <= '9')
				{
					return hexChar - '0';
				}
				else if (hexChar >= 'a' && hexChar <= 'f')
				{
					return 10 + hexChar - 'a';
				}
				else if (hexChar >= 'A' && hexChar <= 'F')
				{
					return 10 + hexChar - 'A';
				}
				else
				{
					assert(0);
					return 0;
				}
			}

			inline uint16 CGUIDHelper::HexCharToUint16(char hexChar)
			{
				return HexCharToUint8(hexChar);
			}

			inline uint32 CGUIDHelper::HexCharToUint32(char hexChar)
			{
				return HexCharToUint8(hexChar);
			}

		}
	}
}

inline CryGUID operator"" _uqs_guid(const char* szGUID, size_t)
{
	return uqs::client::internal::CGUIDHelper::FromString(szGUID);
}
