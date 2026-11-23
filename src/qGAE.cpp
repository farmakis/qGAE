//##########################################################################
//#                                                                        #
//#                CLOUDCOMPARE PLUGIN: qGAE                              #
//#                                                                        #
//#  This program is free software; you can redistribute it and/or modify  #
//#  it under the terms of the GNU General Public License as published by  #
//#  the Free Software Foundation; version 2 of the License.               #
//#                                                                        #
//#  This program is distributed in the hope that it will be useful,       #
//#  but WITHOUT ANY WARRANTY; without even the implied warranty of        #
//#  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the         #
//#  GNU General Public License for more details.                          #
//#                                                                        #
//#                             COPYRIGHT: Ioannis Farmakis                #
//#                                                                        #
//##########################################################################

#include "qGAE.h"

// Local
#include "qGAEDialog.h"
#include "qGAEDisclaimerDialog.h"
#include "qGAEProcess.h"

// Qt
#include <QMainWindow>

// qCC_glWindow
#include <ccGLWindowInterface.h>

qGAE::qGAE( QObject* parent )
	: QObject( parent )
	, ccStdPluginInterface( ":/CC/plugin/qGAE/info.json" )
	, m_action( nullptr )
{
}

void qGAE::onNewSelection( const ccHObject::Container& selectedEntities )
{
	if ( m_action == nullptr )
	{
		return;
	}

	// enable the action only if at least one entity is selected
	m_action->setEnabled( selectedEntities.size() == 1 
						&& selectedEntities[0]->isA(CC_TYPES::POINT_CLOUD) );
}

QList<QAction*> qGAE::getActions()
{
	if ( !m_action )
	{
		m_action = new QAction( getName(), this );
		m_action->setToolTip( getDescription() );
		m_action->setIcon( getIcon() );

		connect( m_action, &QAction::triggered, this, &qGAE::doAction );
	}

	return { m_action };
}

void qGAE::doAction()
{
	//m_app should have already been initialized by CC when plugin is loaded!
	assert(m_app);

	//disclaimer accepted?
	if (!qGAEDisclaimerDialog::show(m_app))
		return;
	
	//check valid window
	if (!m_app->getActiveGLWindow())
	{
		//invalid pointer error
		m_app->dispToConsole("[GAE] Ccould not find the CloudCompare window. Abort!", ccMainAppInterface::ERR_CONSOLE_MESSAGE);
	}
	
	if ( !m_dlg )
	{
		//bind GUI events
		m_dlg = new qGAEDialog( m_app, m_app->getMainWindow() );

		//general
		connect(m_dlg->closeButton, &QAbstractButton::clicked, m_dlg, &QDialog::close);
		connect( m_dlg, &qGAEDialog::undoRequested, this, &qGAE::onUndo );
		connect( m_dlg, &QDialog::finished, this, [this]() { m_dlg->saveParamsToPersistentSettings(); } );

		// sub-dialogs
		connect( m_dlg, &qGAEDialog::voxelRequested, this, &qGAE::onCreateVoxelGrid );
		connect( m_dlg, &qGAEDialog::featuresRequested, this, &qGAE::onComputeFeatures );
		connect( m_dlg, &qGAEDialog::runRequested,   this, &qGAE::onRunPartition );

		// visualization
		connect( m_dlg->rgbButton, &QPushButton::clicked, this, &qGAE::onSetRandomRGB );
	}

	m_dlg->linkWith(m_app->getActiveGLWindow());

	// begin
	launch();
}

void qGAE::onCreateVoxelGrid()
{
	QString errorMessage;
	m_dlg->setEnabled( false ); // no re-entrancy while processing
	if  (!qGAEProcess::CreateVoxelGrid( *m_dlg, m_session, errorMessage, true, m_app->getMainWindow(), m_app ))
	{
		m_app->dispToConsole( errorMessage.isEmpty() ? QStringLiteral( "[GAE] Voxel grid creation failed" ) : errorMessage,
		                      ccMainAppInterface::ERR_CONSOLE_MESSAGE );
	}

	m_dlg->setEnabled( true );
	m_dlg->setSessionActive( m_session.isActive() );
	updateFeatureLevelLimit();
}

void qGAE::onComputeFeatures()
{
	QString errorMessage;
	m_dlg->setEnabled( false );
	if (!qGAEProcess::ComputeFeatures( *m_dlg, m_session, errorMessage, true, m_app->getMainWindow(), m_app ))
	{
		m_app->dispToConsole( errorMessage.isEmpty() ? QStringLiteral( "[GAE] Features computation failed" ) : errorMessage,
		                      ccMainAppInterface::ERR_CONSOLE_MESSAGE );
	}

	m_dlg->setEnabled( true );
	m_dlg->setSessionActive( m_session.isActive() );
}

void qGAE::onRunPartition()
{
	QString errorMessage;
	m_dlg->setEnabled( false );
	if (!qGAEProcess::RunPartition( *m_dlg, m_session, errorMessage, true, m_app->getMainWindow(), m_app ))
	{
		m_app->dispToConsole( errorMessage.isEmpty() ? QStringLiteral( "[GAE] Partition failed" ) : errorMessage,
		                      ccMainAppInterface::ERR_CONSOLE_MESSAGE );
	}

	m_dlg->setEnabled( true );
	m_dlg->setSessionActive( m_session.isActive() );
	updateFeatureLevelLimit();
}

void qGAE::onSetRandomRGB()
{
	int level = m_session.nag.numLevels() - 1;
	if (level < 1)
	{
		m_app->dispToConsole("[GAE] There are no graph levels to visualize!", ccMainAppInterface::ERR_CONSOLE_MESSAGE);
		return;
	}

	QString sfName = QString("Level %1").arg(level);
	m_session.nag[0]->convertLabelsToRandomRGB(sfName.toStdString());

	m_app->updateUI();
    m_app->refreshAll(); 
}

void qGAE::onUndo()
{
	// only the 
	popLastLevel();
	updateFeatureLevelLimit();
}

//Begin GAE 
bool qGAE::launch()
{
	//check valid gl window
	if (!m_app->getActiveGLWindow())
	{
		//invalid pointer error
		m_app->dispToConsole("[GAE] Could not find the CloudCompare window. Abort!", ccMainAppInterface::ERR_CONSOLE_MESSAGE);
		return false;
	}

	//refresh window
	m_app->getActiveGLWindow()->redraw(true, false);

	//start GUI
	m_app->registerOverlayDialog(m_dlg, Qt::TopRightCorner);
	m_dlg->start();

	// activate session helper struct
	m_dlg->setSessionActive( m_session.isActive() );

	return true;
}

void qGAE::popLastLevel()
{
	// nothing to undo: no session, or only the base level remains
    if (!m_session.isActive() || m_session.nag.size() < 2)
        return;

	m_session.nag.deleteAllScalarFields(m_session.nag.numLevels() - 1);

	m_app->removeFromDB(m_session.nag.back());
	m_session.nag.pop_back();

	// update session on the new last level
	Data* newLast = m_session.nag.back();
	m_session.lastId  = newLast ? newLast->getUniqueID() : 0;
}

void qGAE::updateFeatureLevelLimit()
{
	// features of the last level can only be computed from the same or lower levels
	m_dlg->setMaxFeatureLevel( m_session.nag.numLevels() - 1 );
}
