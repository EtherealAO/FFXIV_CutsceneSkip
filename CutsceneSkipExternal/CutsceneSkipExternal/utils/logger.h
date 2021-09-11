#pragma once

namespace shared
{
    struct logger_t
    {
        bool setup( std::string_view console_name )
        {
            if ( !LI_FN( AllocConsole )() )
                return false;

            LI_FN( freopen_s )( reinterpret_cast<_iobuf**>( stdin ), xorstr_( "CONIN$" ), xorstr_( "r" ), stdin );
            LI_FN( freopen_s )( reinterpret_cast<_iobuf**>( stdout ), xorstr_( "CONOUT$" ), xorstr_( "w" ), stdout );
            LI_FN( freopen_s )( reinterpret_cast<_iobuf**>( stderr ), xorstr_( "CONOUT$" ), xorstr_( "w" ), stderr );

            LI_FN( SetConsoleTitleA )( console_name.data() );
            return true;
        }

        void destroy()
        {
            LI_FN( fclose )( stdin );
            LI_FN( fclose )( stdout );
            LI_FN( fclose )( stderr );
            LI_FN( FreeConsole )();
        }

        void log( std::string_view msg )
        {
            std::cout << msg << std::endl;
        }
    };

    inline logger_t logger;
}
#pragma once
