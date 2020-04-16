#pragma once

#include <uchar.h>
#include <stdlib.h>

#ifdef _WIN32
#include <yvals.h>
#endif

#ifdef _WIN32
#define USTR16( STR ) L##STR
#else
#define USTR16( STR ) u##STR
#endif

#include <stdint.h>

namespace Nix
{
    // msvc10		: wchar_t	text[] = WSTR16("Hello,World!");
	// gcc/clang	: char16_t	text[] = WSTR16("Hello,World!");

	//************************************
	// Method:    utf82ucsle
	// FullName:  appui::utf82ucsle
	// Access:    public 
	// Returns:   uint32
	// Qualifier: UTF8 到 Unicode转换方法，渲染文字时用
	// Parameter: const char * _pUTF8	utf8 数据
	// Parameter: uint32 _nDataLen		utf8 长度
	// Parameter: char * _pUnicode		unicode输出buff
	// Parameter: uint32 _nBufferSize	输出buff的长度

    uint32_t utf82ucsle(const char *utf8, uint32_t len, char **unicode);
    uint32_t ucsle2utf8(const char *unic, size_t len, char **utf);
    uint32_t ucsle2gbk(const char *unic, size_t len, char **ascii);
    uint32_t gbk2utf8(const char *acsii, size_t len, char **utf);
    void clear_conv();

    // 从网上抄的一段代码改的
    // 用于unicode字串查找，作用类似于 strstr/wcsstr
	char16_t *ucsstr( const char16_t *s1, const char16_t *s2 );
    // 计算unicode长度
	int ucslen( const char16_t *str );
	// 复制字串
	void ucsncpy( char16_t *dest, const char16_t *src, size_t count);

    class APHasher {
	private:
		uint64_t value;
	public:
		APHasher() : value(0xAAAAAAAA) {}
		operator uint64_t () const { return value; }

		void hash(const void* data, size_t length) 
        {
			for ( size_t i = 0; i < length; ++i )
			{
				uint8_t v = *((const uint8_t *)data + i);
				if (i & 1)
					value ^= ((value << 7) ^ v * (value >> 3));
				else
					value ^= ((value << 11) ^ v * (value >> 5));
			}
		}
	};
}