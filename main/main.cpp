#undef QT_NO_DEBUG_OUTPUT
#include "MainWindow.h"
#include "SABUtils/FileUtils.h"
#include <QApplication>
#include <QDebug>

int main( int argc, char** argv )
{
    QApplication appl( argc, argv );
    Q_INIT_RESOURCE( application );

    QString lFileName;
    for( int ii = 1; ii < argc; ++ii )
    {
        if ( std::strncmp( argv[ ii ], "-xml", 4 ) == 0 )
        {
            if ( ++ii < argc )
                lFileName = argv[ ii ];
        }
    }

    CMainWindow dlg;
    dlg.mLoadFromXML( lFileName );
    return dlg.exec();
}

