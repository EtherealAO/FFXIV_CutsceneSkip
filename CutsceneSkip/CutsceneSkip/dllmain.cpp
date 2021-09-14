// dllmain.cpp : 定义 DLL 应用程序的入口点。
#include <chrono>
#include <thread>
#include <regex>
#include "framework.h"

HANDLE g_handle = INVALID_HANDLE_VALUE;
std::uintptr_t g_address;
BYTE original_bytes[ 2 ][ 2 ] = { { 0x75, 0x33 }, { 0x74, 0x18 } };
BYTE nop[ 2 ] = { 0x90, 0x90 };
bool g_enabled = false;
bool g_initialized = false;

extern "C" __declspec(dllexport) int initialize( int pid )
{
    g_handle = LI_FN( OpenProcess )( PROCESS_ALL_ACCESS, false, pid );
    if ( g_handle == INVALID_HANDLE_VALUE )
        return 0;

    g_address = shared::memory.find_pattern( g_handle, xorstr_( "ffxiv_dx11.exe" ), xorstr_( "48 8B 01 8B D7 FF 90 ? ? ? ? 84 C0 ? ? 48 8B 0D ? ? ? ? BA ? ? ? ? 48 83 C1 10 E8 ? ? ? ? 83 78 20 00 ? ?" ) );
    if ( !g_address )
        return -1;

    BYTE read_bytes[ 2 ][ 2 ] = {};
    LI_FN( ReadProcessMemory )( g_handle, reinterpret_cast<LPCVOID>( g_address + 13 ), read_bytes[ 0 ], sizeof( read_bytes[ 0 ] ), nullptr );
    LI_FN( ReadProcessMemory )( g_handle, reinterpret_cast<LPCVOID>( g_address + 13 + 27 ), read_bytes[ 1 ], sizeof( read_bytes[ 1 ] ), nullptr );

    if ( !LI_FN( WriteProcessMemory )( g_handle, reinterpret_cast<LPVOID>( g_address + 13 ), nop, sizeof( nop ), nullptr ) && !LI_FN( WriteProcessMemory )( g_handle, reinterpret_cast<LPVOID>( g_address + 13 + 27 ), nop, sizeof( nop ), nullptr ) )
        return -2;

    g_enabled = true;
    g_initialized = true;

    return 1;
}

extern "C" __declspec(dllexport) int destroy()
{
    if ( g_handle == INVALID_HANDLE_VALUE )
        return 0;

    if ( !g_address )
        return -1;

    if ( !LI_FN( WriteProcessMemory )( g_handle, reinterpret_cast<LPVOID>( g_address + 13 ), original_bytes[ 0 ], sizeof( original_bytes[ 0 ] ), nullptr ) && !LI_FN( WriteProcessMemory )( g_handle, reinterpret_cast<LPVOID>( g_address + 13 + 27 ), original_bytes[ 1 ], sizeof( original_bytes[ 1 ] ), nullptr ) )
        return -2;

    LI_FN( CloseHandle )( g_handle );
    g_handle = INVALID_HANDLE_VALUE;
    g_enabled = false;
    g_initialized = false;

    return 1;
}

extern "C" __declspec(dllexport) int on_read_log( const char* msg, const char* zone )
{
    if ( !g_initialized )
        return -1;

    // Every 3 bytes is a chinese character
    std::string_view _zone( zone );
    std::string_view _msg( msg );
    if ( _zone.find( xorstr_( "\xE5\xB8\x9D\xE5\x9B\xBD\xE5\x8D\x97\xE6\x96\xB9\xE5\xA0\xA1" ) ) != std::string_view::npos || // 帝国南方堡 
        _zone.find( xorstr_( "\xE5\xA4\xA9\xE5\xB9\x95\xE9\xAD\x94\xE5\xAF\xBC\xE5\x9F\x8E" ) ) != std::string_view::npos || //天幕魔导城
        _zone.find( xorstr_( "\u5E1D\u56FD\u5357\u65B9\u5821" ) ) != std::string_view::npos || // 帝国南方堡 
        _zone.find( xorstr_( "\u5929\u5E55\u9B54\u5BFC\u57CE" ) ) != std::string_view::npos ) //天幕魔导城
    {
        // 00:0039:离开了休
        if ( !g_enabled && ( _msg.find( xorstr_( "00:0039:\xE7\xA6\xBB\xE5\xBC\x80\xE4\xBA\x86\xE4\xBC\x91" ) ) != std::string_view::npos || _msg.find( xorstr_( "00:0039:\u79BB\u5F00\u4E86\u4F11" ) ) != std::string_view::npos ) )
        {
            g_enabled = true;

            LI_FN( WriteProcessMemory )( g_handle, reinterpret_cast<LPVOID>( g_address + 13 ), nop, sizeof( nop ), nullptr );
            LI_FN( WriteProcessMemory )( g_handle, reinterpret_cast<LPVOID>( g_address + 13 + 27 ), nop, sizeof( nop ), nullptr );
        }

        if ( !g_enabled )
        {
            return 2;
        }

        // 00:0839:选用
        /*if ( strstr( msg, xorstr_( "00:0839:\xE9\x80\x89\xE7\x94\xA8" ) ) && g_enabled )
        {
            g_enabled = false;
            // on_read_log can't be called without the plugin being fully initialized
            // so this is safe to call without a if-statement
            LI_FN( WriteProcessMemory )( g_handle, reinterpret_cast<LPVOID>( g_address + 13 ), original_bytes[ 0 ], sizeof( original_bytes[ 0 ] ), nullptr );
            LI_FN( WriteProcessMemory )( g_handle, reinterpret_cast<LPVOID>( g_address + 13 + 27 ), original_bytes[ 1 ], sizeof( original_bytes[ 1 ] ), nullptr );
            return 0;
        }*/

        // 00:0839:队伍
        if ( g_enabled && ( _msg.find( xorstr_( "00:0839:\xE9\x98\x9F\xE4\xBC\x8D" ) ) != std::string_view::npos || _msg.find( xorstr_( "00:0839:\u961F\u4F0D" ) ) != std::string_view::npos ) )
        {
            g_enabled = false;
            // on_read_log can't be called without the plugin being fully initialized
            // so this is safe to call without a if-statement
            LI_FN( WriteProcessMemory )( g_handle, reinterpret_cast<LPVOID>( g_address + 13 ), original_bytes[ 0 ], sizeof( original_bytes[ 0 ] ), nullptr );
            LI_FN( WriteProcessMemory )( g_handle, reinterpret_cast<LPVOID>( g_address + 13 + 27 ), original_bytes[ 1 ], sizeof( original_bytes[ 1 ] ), nullptr );
            return 2;
        }

        return 1;
    }

    if ( g_enabled )
    {
        g_enabled = false;
        // on_read_log can't be called without the plugin being fully initialized
        // so this is safe to call without a if-statement
        LI_FN( WriteProcessMemory )( g_handle, reinterpret_cast<LPVOID>( g_address + 13 ), original_bytes[ 0 ], sizeof( original_bytes[ 0 ] ), nullptr );
        LI_FN( WriteProcessMemory )( g_handle, reinterpret_cast<LPVOID>( g_address + 13 + 27 ), original_bytes[ 1 ], sizeof( original_bytes[ 1 ] ), nullptr );
    }

    return 0;
}
