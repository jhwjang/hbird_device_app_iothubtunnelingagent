#ifndef __HBIRDCRYPTO_HBIRDCRYPTO_H__
#define __HBIRDCRYPTO_HBIRDCRYPTO_H__
#pragma once


#if defined(_WINDOWS_) || defined(_WIN32) || defined(_WIN64)
	#define _HBIRD_WIN
	#define __FILENAME__ (strrchr(__FILE__, '\\') ? strrchr(__FILE__, '\\') + 1 : __FILE__)
#elif defined(__GNUC__) || defined(__linux__)
	#define _HBIRD_LINUX
	#define __FILENAME__ (strrchr(__FILE__, '/') ? strrchr(__FILE__, '/') + 1 : __FILE__)
#elif defined(__APPLE__)
	#define _HBIRD_APPLE
#endif

#define CRYPTO_VERSION_STRING "0.0.3"
#define CRYPTO_VERSION_MAJOR 0
#define CRYPTO_VERSION_MINOR 0
#define CRYPTO_VERSION_PATCH 3
#define CRYPTO_VERSION_QUALIFIER
#define CRYPTO_VERSION_HEXA	((CRYPTO_VERSION_MAJOR << 24) | \
							(CRYPTO_VERSION_MINOR << 16) |  \
							(CRYPTO_VERSION_PATCH << 8))

#define IN 
#define OUT
#define INOUT 

#ifdef _HBIRD_WIN
	#define HBIRDCRYPTO_ATTRS(x)
#elif defined(_HBIRD_LINUX)
	#define HBIRDCRYPTO_ATTRS(x) __attribute__(x)
#endif


////////////////////////////////////////
/// crypt key size (bits)
///
#define __CRYPT_SIZE_128__	128
#define __CRYPT_SIZE_256__ 256
#define __CRYPT_SIZE_512__ 512


#if defined(HBIRDCRYPTODLL_EXPORTS)
#		define _HBIRDCRYPTODLL_EXPORTS 	__declspec( dllexport )
#else
#		define _HBIRDCRYPTODLL_EXPORTS
#endif

#if 1
#ifdef __cplusplus
extern "C" {
#endif


typedef enum {
	eNone = 0,
	eFilePath = 1,
	eString,
	ePadding = 0x7FFFFFFF
} ePasswordInputType;

typedef struct HBirdCryptoStruct {

	size_t				mCryptSize;	// unit: bits
	
	void*				pPW;
	void*				pSaltStruct;
} HBirdCrypto, *pHBirdCrypto;



/******************************************************************************
 * Function Name : HBirdCrypto_Create
 * 
 * Return Value Description
 *	NULL : error
 *
 * Comment :
******************************************************************************/
_HBIRDCRYPTODLL_EXPORTS pHBirdCrypto HBirdCrypto_Create(IN const size_t CryptSize,
	IN const ePasswordInputType eType,
	IN const char* pPassword,
	IN const size_t nPasswordLength);

/******************************************************************************
 * Function Name : HBirdCrypto_Destroy
 *
 * Return Value Description
 *	NULL : void
 *
 * Comment :
******************************************************************************/
_HBIRDCRYPTODLL_EXPORTS void HBirdCrypto_Destroy(IN pHBirdCrypto pCrypto);


_HBIRDCRYPTODLL_EXPORTS const char* HBirdCrypto_version_str(void);
_HBIRDCRYPTODLL_EXPORTS int HBirdCrypto_version_cmp(IN const int major, IN const int minor, IN const int patch);

_HBIRDCRYPTODLL_EXPORTS int HBirdCrypto_gen_rand(INOUT char* randValue, IN const size_t length);
_HBIRDCRYPTODLL_EXPORTS int HBirdCrypto_get_UUID(INOUT char* uuid, IN const size_t length);


/******************************************************************************
 * Function Name : encrypt_Text_C()
 *
 * Return Value Description
 *	NULL : error
 *
 * Comment :
******************************************************************************/
_HBIRDCRYPTODLL_EXPORTS int encrypt_Text_C(IN pHBirdCrypto pCrypto,
											IN const char* pPlainText, IN const int nPlainTextSize,
											INOUT char** pEncText, INOUT int* nEncTextSize,
											IN const char* pPassword, IN const int nPasswordLength);
/******************************************************************************
 * Function Name : decrypt_Text_C()
 *
 * Return Value Description
 *	NULL : error
 *
 * Comment :
******************************************************************************/
_HBIRDCRYPTODLL_EXPORTS int decrypt_Text_C(IN pHBirdCrypto pCrypto, 
										IN const char* pEncText, IN const int nEncTextSize,
										INOUT char** pPlainText, INOUT int* nPlainTextSize,
										IN const char* pPassword, IN const int nPasswordLength);
/******************************************************************************
 * Function Name : encrypt_File_C()
 *
 * Return Value Description
 *	NULL : error
 *
 * Comment :
******************************************************************************/
_HBIRDCRYPTODLL_EXPORTS int encrypt_File_C(IN pHBirdCrypto pCrypto,
										IN const char* pPlainFilePath, IN const int nPlainFilePathSize,
										IN const char* pEncFilePath, IN const int nEncFilePathSize,
										IN const char* pPassword, IN const int nPasswordLength);
/******************************************************************************
 * Function Name : decrypt_File_C()
 *
 * Return Value Description
 *	NULL : error
 *
 * Comment :
******************************************************************************/
_HBIRDCRYPTODLL_EXPORTS int decrypt_File_C(IN pHBirdCrypto pCrypto,
										IN const char* pEncFilePath, IN const int nEncFilePathSize,
										IN const char* pPlainFilePath, IN const int nPlainFilePathSize,
										IN const char* pPassword, IN const int nPasswordLength);


#ifdef __cplusplus
}
#endif
#endif

#endif  //__HBIRDCRYPTO_HBIRDCRYPTO_H__
