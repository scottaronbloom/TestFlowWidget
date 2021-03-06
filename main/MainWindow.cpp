#include "MainWindow.h"
#include "ui_MainWindow.h"  
#include "SABUtils/QtDumper.h"

#include <QTimer>
#include <QInputDialog>
#include <QFileDialog>
#include <QStandardItemModel>
#include <QDebug>
#include <QMessageBox>
#include <algorithm>

QList< int > mergeStates( const QList< QList< int > >& xStates )
{
    QList< int > lRetVal;

    for ( int ii = 0; ii < xStates.count(); ++ii )
    {
        for ( int jj = 0; jj < xStates[ ii ].count(); ++jj )
        {
            if ( lRetVal.indexOf( xStates[ ii ][ jj ] ) == -1 )
                lRetVal << xStates[ ii ][ jj ];
        }
    }
    return lRetVal;
}


CMainWindow::CMainWindow( QWidget* parent )
    : QDialog( parent ),
    fModel( new QStandardItemModel( this ) ),
    fImpl( new Ui::CMainWindow )
{
    fImpl->setupUi( this );
    fImpl->widgetDump->setModel( fModel );

    fImpl->takeButton->setEnabled( false );
    fImpl->placeButton->setEnabled( false );
    fImpl->hideButton->setEnabled( false );
    fImpl->unhideButton->setEnabled( false );
    fImpl->disableButton->setEnabled( false );
    fImpl->enableButton->setEnabled( false );
    fImpl->elideTextInFlowWidget->setChecked( fImpl->flowWidget->mElideText() );
    fImpl->summarizeStatus->setChecked( fImpl->flowWidget->mSummarizeStatus() );
    fImpl->defaultSummarize->setChecked( true );

    connect( fImpl->elideTextInFlowWidget, &QCheckBox::clicked,
             [this]()
    {
        fImpl->flowWidget->mSetElideText( fImpl->elideTextInFlowWidget->isChecked() );
    } );

    connect( fImpl->summarizeStatus, &QGroupBox::clicked, fImpl->flowWidget, &CFlowWidget::mSetSummarizeStatus );

    connect( fImpl->defaultSummarize, &QCheckBox::clicked,
             [this]()
    {
        fImpl->flowWidget->mSetMergeStatesFunction( {} );
    } );

    connect( fImpl->parentOnlySummarize, &QCheckBox::clicked,
             [this]()
    {
        fImpl->flowWidget->mSetMergeStatesFunction(
            []( CFlowWidgetItem* /*xParent*/, const QList< int >& lParentLocalStates, const QList< QList< int > >& xChildStates )
        {
            if ( !lParentLocalStates.isEmpty() )
                return lParentLocalStates;
            return mergeStates( QList< QList< int > >() << lParentLocalStates << xChildStates );
        } );
    } );
    connect( fImpl->allSummarize, &QCheckBox::clicked,
             [this]()
    {
        fImpl->flowWidget->mSetMergeStatesFunction(
            []( CFlowWidgetItem* /*xParent*/, const QList< int >& lParentLocalStates, const QList< QList< int > >& xChildStates )
        {
            return mergeStates( QList< QList< int > >() << lParentLocalStates << xChildStates );
        } );
    } );
    connect( fImpl->onlyErrorSummarize, &QCheckBox::clicked,
             [this]()
    {
        fImpl->flowWidget->mSetMergeStatesFunction(
            []( CFlowWidgetItem* /*xParent*/, const QList< int >& lParentLocalStates, const QList< QList< int > >& xChildStates )
        {
            if ( !lParentLocalStates.isEmpty() )
                return lParentLocalStates;

            auto lRetVal = mergeStates( QList< QList< int > >() << lParentLocalStates << xChildStates );
            if ( lRetVal.indexOf( CFlowWidget::EStates::eRunCompletedWithError ) != -1 )
                lRetVal = QList< int >() << CFlowWidget::EStates::eRunCompletedWithError;
            else
                lRetVal.clear();
            return lRetVal;
        } );
    } );
    connect( fImpl->mostSevereSummarize, &QCheckBox::clicked,
             [this]()
    {
        fImpl->flowWidget->mSetMergeStatesFunction(
            []( CFlowWidgetItem* /*xParent*/, const QList< int >& lParentLocalStates, const QList< QList< int > >& xChildStates )
        {
            if ( !lParentLocalStates.isEmpty() )
                return lParentLocalStates;

            auto lRetVal = mergeStates( QList< QList< int > >() << lParentLocalStates << xChildStates );
            std::sort( lRetVal.begin(), lRetVal.end() );
            if ( !lRetVal.isEmpty() )
                return QList< int >() << lRetVal.back();
            return lRetVal;
        } );
    } );

    connect( fImpl->alignStatus, &QCheckBox::clicked, fImpl->flowWidget, &CFlowWidget::mSetAlignStatus );

    connect( fImpl->takenItems, &QListWidget::itemSelectionChanged,
             [this]()
    {
        fImpl->placeButton->setEnabled( (fImpl->takenItems->selectedItems().size() == 1) );
    } );

    connect( fImpl->hiddenItems, &QListWidget::itemSelectionChanged,
             [this]()
    {
        fImpl->unhideButton->setEnabled( fImpl->hiddenItems->selectedItems().size() == 1 );
    } );

    connect( fImpl->disabledItems, &QListWidget::itemSelectionChanged,
             [this]()
    {
        fImpl->enableButton->setEnabled( fImpl->disabledItems->selectedItems().size() == 1 );
    } );

    connect( fImpl->statusList, &QListWidget::itemChanged, this, &CMainWindow::slotStatusItemSelected );

    connect( fImpl->dumpFlowWidgetBtn, &QPushButton::clicked,
             [this]()
    {
        mDumpFlowWidget();
    } );
    connect( fImpl->demoFlowWidget, &QPushButton::clicked,
             [this]()
    {
        mDemoFlowWidget();
    } );

    connect( fImpl->loadFromXML, &QPushButton::clicked,
             [this]()
    {
        mLoadFromXML();
    } );

    connect( fImpl->addCustomStatus, &QPushButton::clicked,
             [this]()
    {
        auto lID = fImpl->flowWidget->mGetNextStatusID();
        auto lDesc = QInputDialog::getText( this, tr( "Enter Description:" ), tr( "Enter Description:" ) );
        if ( lDesc.isEmpty() )
            return;
        auto lPath = QFileDialog::getOpenFileName( this, tr( "Select Image File:" ), QString(), tr( "Images (*.png *.xpm *.jpg);;AllFiles(*.*)" ) );
        if ( lPath.isEmpty() )
            return;
        QIcon lIcon = QIcon( lPath );
        fImpl->flowWidget->mRegisterStateStatus( lID, lDesc, lIcon );
        this->mLoadStatus( SRegisteredStatusInfo( lID, lDesc, lIcon, false ) );
    } );

    connect( fImpl->setText, &QAbstractButton::clicked,
             [this]()
    {
        auto selected = fImpl->flowWidget->mSelectedItem();
        if ( !selected )
            return;
        auto txt = fImpl->selected->text();
        auto pos = txt.lastIndexOf( '.' );
        if ( pos != -1 )
            txt = txt.mid( pos + 1 );
        selected->mSetText( txt );
    } );
    connect( fImpl->setToolTip, &QAbstractButton::clicked,
             [this]()
    {
        auto selected = fImpl->flowWidget->mSelectedItem();
        if ( !selected )
            return;
        selected->mSetToolTip( fImpl->toolTip->text() );
    } );

    connect( fImpl->flowWidget, &CFlowWidget::sigFlowWidgetItemSelected,
             [this]( CFlowWidgetItem* xItem, bool xSelected )
    {
        slotFlowWidgetItemSelected( xItem, xSelected );
    } );

    connect( fImpl->flowWidget, &CFlowWidget::sigFlowWidgetItemDoubleClicked,
             [this]( CFlowWidgetItem* xItem )
    {
        QString lText;
        if ( xItem )
        {
            lText = xItem->mFullText();
        }
        fImpl->doubleClicked->setText( lText );
    } );

    connect( fImpl->flowWidget, &CFlowWidget::sigFlowWidgetItemHovered,
             [this]( CFlowWidgetItem* xItem )
    {
        QString lText;
        if ( xItem )
        {
            lText = xItem->mFullText();
        }
        fImpl->hovered->setText( lText );
    } );

    connect( fImpl->takeButton, &QAbstractButton::clicked,
             [this]()
    {
        auto xItem = fImpl->flowWidget->mSelectedItem();
        if ( !xItem )
            return;

        fImpl->flowWidget->mTakeItem( xItem );
        auto lwItem = new QListWidgetItem( xItem->mFullText() );
        QVariant lValue;
        lValue.setValue( xItem );
        lwItem->setData( Qt::UserRole + 1, lValue );
        fImpl->takenItems->addItem( lwItem );

        mDumpFlowWidget();
    } );
    connect( fImpl->placeButton, &QAbstractButton::clicked,
             [this]()
    {
        auto lSelectedTaken = fImpl->takenItems->selectedItems();
        if ( lSelectedTaken.count() != 1 )
            return;

        auto lListWidgetItem = lSelectedTaken.front();
        auto lTakenItem = lListWidgetItem->data( Qt::UserRole + 1 ).value< CFlowWidgetItem* >();
        if ( !lTakenItem )
            return;

        delete lListWidgetItem;

        auto lItemSelected = fImpl->flowWidget->mSelectedItem();
        auto lItemParent = lItemSelected ? lItemSelected->mParentItem() : nullptr;

        auto lItems = QStringList();
        if ( lItemSelected != nullptr )
        {
            lItems << tr( "Before Current Selection" ) << tr( "After Current Selection" );
            lItems << tr( "Append as Child of Current Selection" ) << tr( "Insert as First Child of Current Selection" );
        }
        lItems << tr( "Append Top Level Item" ) << tr( "Insert as First Top Level Item" );

        bool aOK = false;
        QString lPrefix;
        if ( lItemSelected )
            lPrefix = tr( "<li>Current Selection: %1</li>" ).arg( lItemSelected->mFullText() );
        if ( lItemParent )
            lPrefix += tr( "<li>Parent of Selection: %1</li>" ).arg( lItemParent->mFullText() );

        if ( !lPrefix.isEmpty() )
            lPrefix = "<ul>" + lPrefix + "</ul>";
        auto lLabel = lPrefix + tr( "Place Where:" );
        auto lSelected = QInputDialog::getItem( this, tr( "Place Where:" ), lLabel, lItems, 0, false, &aOK );
        if ( !aOK )
            return;

        if ( lSelected == tr( "Append Top Level Item" ) )
        {
            fImpl->flowWidget->mAddTopLevelItem( lTakenItem );
        }
        else if ( lSelected == tr( "Insert as First Top Level Item" ) )
        {
            fImpl->flowWidget->mInsertTopLevelItem( 0, lTakenItem );
        }
        else if ( lItemSelected )
        {
            if ( (lSelected == tr( "After Current Selection" )) || (lSelected == tr( "Before Current Selection" )) )
            {
                bool lBefore = lSelected.startsWith( tr( "Before" ) );
                if ( lItemParent )
                    lItemParent->mInsertChild( lItemSelected, lTakenItem, lBefore );
                else
                    fImpl->flowWidget->mInsertTopLevelItem( lItemSelected, lTakenItem, lBefore );
            }
            else if ( lSelected == tr( "Append as Child of Current Selection" ) )
            {
                lItemSelected->mAddChild( lTakenItem );
            }
            else if ( lSelected == tr( "Insert as First Child of Current Selection" ) )
            {
                lItemSelected->mInsertChild( 0, lTakenItem );
            }
        }
        mDumpFlowWidget();
    } );
    connect( fImpl->hideButton, &QAbstractButton::clicked,
             [this]()
    {
        auto xItem = fImpl->flowWidget->mSelectedItem();
        if ( !xItem )
            return;

        xItem->mSetHidden( true );
        auto lwItem = new QListWidgetItem( xItem->mFullText() );
        QVariant lValue;
        lValue.setValue( xItem );
        lwItem->setData( Qt::UserRole + 1, lValue );
        fImpl->hiddenItems->addItem( lwItem );

    } );
    connect( fImpl->unhideButton, &QAbstractButton::clicked,
             [this]()
    {
        auto selected = fImpl->hiddenItems->selectedItems();
        if ( selected.count() != 1 )
            return;

        auto selectedItem = selected.front()->data( Qt::UserRole + 1 ).value< CFlowWidgetItem* >();
        if ( !selectedItem )
            return;

        selectedItem->mSetHidden( false );
        delete selected.front();
    } );

    connect( fImpl->disableButton, &QAbstractButton::clicked,
             [this]()
    {
        auto xItem = fImpl->flowWidget->mSelectedItem();
        if ( !xItem )
            return;

        xItem->mSetDisabled( true );
        auto lwItem = new QListWidgetItem( xItem->mFullText() );
        QVariant lValue;
        lValue.setValue( xItem );
        lwItem->setData( Qt::UserRole + 1, lValue );
        fImpl->disabledItems->addItem( lwItem );

    } );
    connect( fImpl->enableButton, &QAbstractButton::clicked,
             [this]()
    {
        auto selected = fImpl->disabledItems->selectedItems();
        if ( selected.count() != 1 )
            return;

        auto selectedItem = selected.front()->data( Qt::UserRole + 1 ).value< CFlowWidgetItem* >();
        if ( !selectedItem )
            return;

        selectedItem->mSetEnabled( true );
        delete selected.front();
    } );
    slotFlowWidgetItemSelected( fImpl->flowWidget->mSelectedItem(), true );
}

void CMainWindow::slotFlowWidgetItemSelected( CFlowWidgetItem* xItem, bool xSelected )
{
    fImpl->takeButton->setEnabled( xItem );
    fImpl->disableButton->setEnabled( xItem );
    fImpl->hideButton->setEnabled( xItem );

    fImpl->unhideButton->setEnabled( (fImpl->hiddenItems->selectedItems().size() == 1) && (xItem != nullptr) );
    fImpl->enableButton->setEnabled( (fImpl->disabledItems->selectedItems().size() == 1) && (xItem != nullptr) );
    fImpl->placeButton->setEnabled( (fImpl->takenItems->selectedItems().size() == 1) );

    fImpl->attributes->clear();
    disconnect( fImpl->statusList, &QListWidget::itemChanged, this, &CMainWindow::slotStatusItemSelected );

    QList< int > lSelectedIDs;
    if ( xItem )
    {
        QString lText;
        if ( xSelected )
        {
            lText = xItem->mFullText();
        }
        fImpl->selected->setText( lText );
        fImpl->toolTip->setText( xItem->mToolTip() );
        fImpl->uiClassName->setText( xItem->mGetAttribute( "ui" ) );
        fImpl->stepID->setText( xItem->mStepID() );
        fImpl->tclProcName->setText( xItem->mGetAttribute( "tclproc" ) );

        auto lAllAttributes = xItem->mGetAttributes();
        for ( auto&& ii : lAllAttributes )
        {
            auto lKey = ii.first;
            auto lValue = ii.second;
            auto lCurr = new QTreeWidgetItem( fImpl->attributes, QStringList() << lKey << lValue );
            (void)lCurr;
        }

        lText = xItem->mDump( true, false, false );
        fImpl->jsonDump->setPlainText( lText );

        lSelectedIDs = xItem->mStateStatuses( true );
    }

    for ( int ii = 0; ii < fImpl->statusList->count(); ++ii )
    {
        auto item = fImpl->statusList->item( ii );
        if ( !item )
            continue;
        auto statusID = item->type() - QListWidgetItem::ItemType::UserType;
        item->setCheckState( (lSelectedIDs.indexOf( statusID ) == -1) ? Qt::Unchecked : Qt::Checked );
    }

    connect( fImpl->statusList, &QListWidget::itemChanged, this, &CMainWindow::slotStatusItemSelected );
}

CMainWindow::~CMainWindow()
{
}

void CMainWindow::mDemoFlowWidget()
{
    fImpl->flowWidget->mClear();
    int stateID = 0;
    {
        auto flowItem = fImpl->flowWidget->mAddTopLevelItem( QString( "State_%1" ).arg( stateID++ ), "FlowItem 1" );
        flowItem->mSetIcon( QIcon( ":/Entity.png" ) );
        auto icon = flowItem->mIcon();
        Q_ASSERT( !icon.isNull() );
        auto subFlowItem1 = new CFlowWidgetItem( QString( "State_%1" ).arg( stateID++ ), "SubFlowItem 1-1", flowItem );
        subFlowItem1->mSetIcon( QIcon( ":/Entity.png" ) );
        auto subFlowItem12 = new CFlowWidgetItem( QString( "State_%1" ).arg( stateID++ ), "SubFlowItem 1-1-2", QIcon( ":/Entity.png" ), subFlowItem1 );
        auto subFlowItem2 = new CFlowWidgetItem( QString( "State_%1" ).arg( stateID++ ), "SubFlowItem 1-2", flowItem );
    }

    {
        auto  flowItem = new CFlowWidgetItem( QString( "State_%1" ).arg( stateID++ ), "FlowItem 2", fImpl->flowWidget );
        auto subFlowItem1 = new CFlowWidgetItem( QString( "State_%1" ).arg( stateID++ ), "SubFlowItem 2-1", flowItem );
        auto subFlowItem2 = fImpl->flowWidget->mAddItem( QString( "State_%1" ).arg( stateID++ ), "SubFlowItem 2-2", flowItem );
        auto subFlowItem12 = new CFlowWidgetItem( QString( "State_%1" ).arg( stateID++ ), "SubFlowItem 2-2-1", QIcon( ":/Entity.png" ), subFlowItem2 );
    }

    {
        auto  flowItem = new CFlowWidgetItem( QString( "State_%1" ).arg( stateID++ ), "FlowItem 3", fImpl->flowWidget );
        auto subFlowItem1 = new CFlowWidgetItem( QString( "State_%1" ).arg( stateID++ ), "SubFlowItem 3-1", flowItem );
        auto subFlowItem2 = new CFlowWidgetItem( QString( "State_%1" ).arg( stateID++ ), "SubFlowItem 3-2", flowItem );
        auto subFlowItem22 = new CFlowWidgetItem( QString( "State_%1" ).arg( stateID++ ), "SubFlowItem 3-2-1", QIcon( ":/Entity.png" ), subFlowItem2 );
    }

    mLoadStatuses();

    QTimer::singleShot( 0, this, [this]() { mDumpFlowWidget(); } );
}

void CMainWindow::mLoadStatuses()
{
    fImpl->statusList->clear();
    auto lStatuses = fImpl->flowWidget->mGetRegisteredStatuses();
    for ( auto&& ii : lStatuses )
    {
        mLoadStatus( ii );
    }
}

void CMainWindow::mLoadStatus( const SRegisteredStatusInfo& ii )
{
    auto lCurr = new QListWidgetItem( ii.dIcon, ii.dStateDesc, nullptr, QListWidgetItem::ItemType::UserType + ii.dStateID );
    if ( ii.dStateID == CFlowWidget::EStates::eDisabled )
        lCurr->setFlags( lCurr->flags() & ~Qt::ItemIsSelectable );
    lCurr->setFlags( lCurr->flags() | Qt::ItemIsUserCheckable );
    lCurr->setIcon( ii.dIcon );
    lCurr->setCheckState( Qt::Unchecked );
    fImpl->statusList->addItem( lCurr );
}

void CMainWindow::mDumpFlowWidget()
{
    auto lText = fImpl->flowWidget->mDump( false, false );
    fImpl->jsonDump->setPlainText( lText );

    dumpWidgetAndChildren( fImpl->flowWidget, fModel );
    fImpl->widgetDump->expandAll();
    mCollapseWidgetType( QModelIndex() );

    for ( auto ii = 0; ii < fModel->columnCount(); ++ii )
        fImpl->widgetDump->resizeColumnToContents( ii );
}

void CMainWindow::mCollapseWidgetType( const QModelIndex& parent )
{
    if ( parent.isValid() )
    {
        if ( parent.data() == "Widget" )
        {
            auto subIdx = fModel->index( 0, 0, parent );
            fImpl->widgetDump->setExpanded( subIdx, false );
        }
    }
    for ( auto ii = 0; ii < fModel->rowCount( parent ); ++ii )
    {
        auto subIndex = fModel->index( ii, 0, parent );
        mCollapseWidgetType( subIndex );
    }
}

void CMainWindow::slotStatusItemSelected( QListWidgetItem* /*xListWidgetItem*/ )
{
    auto selectedItem = fImpl->flowWidget->mSelectedItem();
    if ( !selectedItem )
        return;

    QList< int > states;
    for ( int ii = 0; ii < fImpl->statusList->count(); ++ii )
    {
        auto item = fImpl->statusList->item( ii );
        if ( !item || (item->checkState() != Qt::Checked) )
            continue;
        states << item->type() - QListWidgetItem::ItemType::UserType;
    }
    selectedItem->mSetStateStatuses( states );
};

void CMainWindow::mLoadFromXML()
{
    auto lFileName = QFileDialog::getOpenFileName( this, tr( "Select File Name" ), QString(), QString( "XML Files (*.xml);;All Files (*.*)" ) );
    if ( lFileName.isEmpty() )
        return;

    mLoadFromXML( lFileName );
}

void CMainWindow::mLoadFromXML( const QString& xFileName )
{
    auto lRetVal = fImpl->flowWidget->mLoadFromXML( xFileName );
    if ( !lRetVal.second.isEmpty() )
    {
        if ( lRetVal.first )
            QMessageBox::information( this, tr( "Warning" ), lRetVal.second );
        else
            QMessageBox::critical( this, tr( "Error" ), lRetVal.second );
    }
    if ( lRetVal.first )
    {
        mLoadStatuses();
        mDumpFlowWidget();
    }
}

