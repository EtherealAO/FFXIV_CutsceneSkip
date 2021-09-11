#pragma once

namespace shared
{
    struct memory_t
    {
        std::vector<uint32_t> get_processes( std::string_view name )
        {
            std::vector<uint32_t> list;

            PROCESSENTRY32 pe32;
            HANDLE process_snap = LI_FN( CreateToolhelp32Snapshot )( TH32CS_SNAPPROCESS, 0 );

            if ( process_snap == INVALID_HANDLE_VALUE )
                return list;

            pe32.dwSize = sizeof( PROCESSENTRY32 );
            if ( LI_FN( Process32First )( process_snap, &pe32 ) )
            {
                if ( pe32.szExeFile == name )
                    list.push_back( pe32.th32ProcessID );

                while ( LI_FN( Process32Next )( process_snap, &pe32 ) )
                {
                    if ( pe32.szExeFile == name )
                        list.push_back( pe32.th32ProcessID );
                }
            }

            LI_FN( CloseHandle )( process_snap );

            return list;
        }

        void attach( uint32_t pid )
        {
            handle = LI_FN( OpenProcess )( PROCESS_ALL_ACCESS, false, pid );
            if ( handle )
            {
                ( printf )( xorstr_( "[+] process attached\n" ) );
            }
        }

        void detach()
        {
            LI_FN( CloseHandle )( handle );
            handle = INVALID_HANDLE_VALUE;
        }

        auto get_handle()
        {
            return handle;
        }

        auto get_module_handle( const char* name )
        {
            HMODULE null = { nullptr };
            HMODULE mods[ 1024 ];
            DWORD cb_needed;

            if ( LI_FN( EnumProcessModules )( handle, mods, sizeof( mods ), &cb_needed ) )
            {
                for ( auto i = 0; i < ( cb_needed / sizeof( HMODULE ) ); i++ )
                {
                    TCHAR szModName[ MAX_PATH ];
                    if ( LI_FN( GetModuleBaseNameA )( handle, mods[ i ], szModName, sizeof( szModName ) / sizeof( TCHAR ) ) )
                    {
                        if ( LI_FN( strcmp )( szModName, name ) == 0 )
                        {
                            return mods[ i ];
                        }
                    }
                }
            }

            return null;
        }

        auto get_module_info( const char* name )
        {
            HMODULE module_handle = get_module_handle( name );
            MODULEINFO info;
            LI_FN( GetModuleInformation )( handle, module_handle, &info, sizeof( info ) );
            return info;
        }

        std::uintptr_t find_pattern( const char* mod, const char* pattern )
        {
            auto mod_info = get_module_info( mod );

            const auto image_size = mod_info.SizeOfImage;
            if ( !image_size )
                return 0;

            const auto base_addr = reinterpret_cast<std::uintptr_t>( mod_info.lpBaseOfDll );

            static auto pattern_to_byte = []( const char* pattern )
            {
                /// Prerequisites
                auto bytes = std::vector<int>{};
                auto start = const_cast<char*>( pattern );
                auto end = const_cast<char*>( pattern ) + strlen( pattern );

                /// Convert signature into corresponding bytes
                for ( auto current = start; current < end; ++current )
                {
                    /// Is current byte a wildcard? Simply ignore that that byte later
                    if ( *current == '?' )
                    {
                        ++current;

                        /// Check if following byte is also a wildcard
                        if ( *current == '?' )
                            ++current;

                        /// Dummy byte
                        bytes.push_back( -1 );
                    } else
                    {
                        /// Convert character to byte on hexadecimal base
                        bytes.push_back( strtoul( current, &current, 16 ) );
                    }
                }
                return bytes;
            };

            auto pattern_bytes = pattern_to_byte( pattern );

            const auto signature_size = pattern_bytes.size();
            const auto signature_bytes = pattern_bytes.data();

            const auto image_bytes = static_cast<byte*>( LI_FN( VirtualAlloc )( nullptr, image_size, MEM_COMMIT, PAGE_EXECUTE_READWRITE ) );
            LI_FN( ReadProcessMemory )( handle, reinterpret_cast<void*>( base_addr ), image_bytes, image_size, nullptr );

            for ( std::size_t i = 0; i < image_size; i++ )
            {
                if ( image_bytes[ i ] == pattern_bytes[ 0 ] )
                {
                    for ( std::size_t j = 1; j < signature_size; j++ )
                    {
                        if ( signature_bytes[ j ] != -1 && signature_bytes[ j ] != image_bytes[ i + j ] )
                            break;

                        if ( j + 1 == signature_size )
                            return base_addr + i;
                    }
                }
            }

            return 0;
        }

        HANDLE handle;
    };

    inline memory_t memory;
}
