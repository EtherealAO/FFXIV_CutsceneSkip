#include "header.h"

int main()
{
    LI_FN( printf )( xorstr_( "[-] searching for ffxiv_dx11.exe...\n" ) );

    auto list = shared::memory.get_processes( xorstr_( "ffxiv_dx11.exe" ) );
    while ( list.empty() )
    {
        LI_FN( Sleep )( 500 );
        list = shared::memory.get_processes( xorstr_( "ffxiv_dx11.exe" ) );
    }
    shared::memory.attach( list.front() );

    auto sig_addr = shared::memory.find_pattern( xorstr_( "ffxiv_dx11.exe" ), xorstr_( "48 8B 01 8B D7 FF 90 ? ? ? ? 84 C0 75 33" ) );

    if ( sig_addr )
    {
        byte original_bytes[ 2 ][ 2 ] = { { 0x75, 0x33 }, { 0x74, 0x18 } };
        byte read_bytes[ 2 ][ 2 ];
        byte nop[ 2 ] = { 0x90, 0x90 };
        byte patch[ 2 ] = { 0x75, 0x18 };
        auto is_patched = [nop]( byte* b1 )
        {
            return *b1 == *nop;
        };

        auto handle = shared::memory.get_handle();
        LI_FN( ReadProcessMemory )( handle, reinterpret_cast<LPCVOID>( sig_addr + 13 ), read_bytes[ 0 ], sizeof( read_bytes[ 0 ] ), nullptr );
        LI_FN( ReadProcessMemory )( handle, reinterpret_cast<LPCVOID>( sig_addr + 13 + 0x1b ), read_bytes[ 1 ], sizeof( read_bytes[ 1 ] ), nullptr );

        if ( !is_patched( read_bytes[ 0 ] ) && !is_patched( read_bytes[ 1 ] ) )
        {
            if ( LI_FN( WriteProcessMemory )( handle, reinterpret_cast<LPVOID>( sig_addr + 13 ), nop, sizeof( nop ), nullptr ) && LI_FN( WriteProcessMemory )( handle, reinterpret_cast<LPVOID>( sig_addr + 13 + 0x1b ), nop, sizeof( nop ), nullptr ) )
                LI_FN( printf )( xorstr_( "[+] patched\n" ) );
        }

        while ( !( ( GetAsyncKeyState )( VK_F10 ) & 1 ) )
        {
            if ( ( GetAsyncKeyState )( VK_INSERT ) & 1 )
            {
                LI_FN( ReadProcessMemory )( handle, reinterpret_cast<LPCVOID>( sig_addr + 13 ), read_bytes[ 0 ], sizeof( read_bytes[ 0 ] ), nullptr );
                LI_FN( ReadProcessMemory )( handle, reinterpret_cast<LPCVOID>( sig_addr + 13 + 0x1b ), read_bytes[ 1 ], sizeof( read_bytes[ 1 ] ), nullptr );

                if ( !is_patched( read_bytes[ 0 ] ) && !is_patched( read_bytes[ 1 ] ) )
                {
                    if ( LI_FN( WriteProcessMemory )( handle, reinterpret_cast<LPVOID>( sig_addr + 13 ), nop, sizeof( nop ), nullptr ) && LI_FN( WriteProcessMemory )( handle, reinterpret_cast<LPVOID>( sig_addr + 13 + 0x1b ), nop, sizeof( nop ), nullptr ) )
                        LI_FN( printf )( xorstr_( "[+] patched\n" ) );
                } else
                {
                    if ( LI_FN( WriteProcessMemory )( handle, reinterpret_cast<LPVOID>( sig_addr + 13 ), original_bytes[ 0 ], sizeof( original_bytes[ 0 ] ), nullptr ) && LI_FN( WriteProcessMemory )( handle, reinterpret_cast<LPVOID>( sig_addr + 13 + 0x1b ), original_bytes[ 1 ], sizeof( original_bytes[ 1 ] ), nullptr ) )
                        LI_FN( printf )( xorstr_( "[+] restored\n" ) );
                }
            }
            std::this_thread::sleep_for( std::chrono::seconds( 1 ) );
        }

        if ( LI_FN( WriteProcessMemory )( handle, reinterpret_cast<LPVOID>( sig_addr + 13 ), original_bytes[ 0 ], sizeof( original_bytes[ 0 ] ), nullptr ) && LI_FN( WriteProcessMemory )( handle, reinterpret_cast<LPVOID>( sig_addr + 13 + 0x1b ), original_bytes[ 1 ], sizeof( original_bytes[ 1 ] ), nullptr ) )
            LI_FN( printf )( xorstr_( "[+] restored\n" ) );

    } else
    {
        LI_FN( printf )( xorstr_( "[!] invalid signature" ) );
        LI_FN( system )( xorstr_( "pause" ) );
    }
    return 0;
}
