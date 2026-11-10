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
	m_action->setEnabled( !selectedEntities.empty() );
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

	qGAEDialog dlg( m_app );
	if ( !dlg.exec() )
	{
		return;
	}

    QString errorMessage;
	ccPointCloud* outputCloud = nullptr; //only necessary for the command line version in fact
	ccHObject* outputGroup = nullptr; //only necessary for the command line version in fact
    if (!qGAEProcess::Compute(dlg, errorMessage, outputCloud, outputGroup, true, m_app->getMainWindow(), m_app))
	{
		if (!errorMessage.isEmpty())
		{
			m_app->dispToConsole(errorMessage, ccMainAppInterface::WRN_CONSOLE_MESSAGE);
		}
		else
		{
			m_app->dispToConsole("[GAE] Completed!", ccMainAppInterface::STD_CONSOLE_MESSAGE);
		}	
	}

	//'Compute' may change some parameters of the dialog
	dlg.saveParamsToPersistentSettings();
}
