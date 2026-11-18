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

#include "qGAEDialog.h"
#include "qGAEProcess.h"

#include <QMainWindow>

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
	if ( m_app == nullptr )
	{
		Q_ASSERT( false );
		return;
	}

	if ( !m_dlg )
	{
		m_dlg = new qGAEDialog( m_app );
		connect( m_dlg, &qGAEDialog::voxelRequested, this, &qGAE::onCreateVoxelGrid );
		connect( m_dlg, &qGAEDialog::featuresRequested, this, &qGAE::onComputeFeatures );
		connect( m_dlg, &qGAEDialog::runRequested,   this, &qGAE::onRunPartition );
		connect( m_dlg, &qGAEDialog::resetRequested, this, &qGAE::onReset );
		connect( m_dlg, &QDialog::finished, this, [this]() { m_dlg->saveParamsToPersistentSettings(); } );
	}

	m_dlg->setSessionActive( m_session.isActive() );

	// non-modal: the 3D view stays usable between two runs
	m_dlg->show();
	m_dlg->raise();
	m_dlg->activateWindow();
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

void qGAE::onReset()
{
	// the levels stay in the DB, we only forget them
	m_session.reset();
	m_dlg->setSessionActive( false );
	updateFeatureLevelLimit();
}

void qGAE::updateFeatureLevelLimit()
{
	// features of the last level can only be computed from lower levels
	m_dlg->setMaxFeatureLevel( m_session.nag.numLevels() - 2 );
}
