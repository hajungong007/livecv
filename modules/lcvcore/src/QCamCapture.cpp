/****************************************************************************
**
** Copyright (C) 2014 Dinu SV.
** (contact: mail@dinusv.com)
** This file is part of Live CV application.
**
** GNU General Public License Usage
** 
** This file may be used under the terms of the GNU General Public License 
** version 3.0 as published by the Free Software Foundation and appearing 
** in the file LICENSE.GPL included in the packaging of this file.  Please 
** review the following information to ensure the GNU General Public License 
** version 3.0 requirements will be met: http://www.gnu.org/copyleft/gpl.html.
**
****************************************************************************/

#include "QCamCapture.hpp"
#include "QCamCaptureThread.hpp"
#include "QStateContainer.hpp"

#include <QTimer>

QCamCapture::QCamCapture(QQuickItem *parent) :
    QMatDisplay(parent),
    m_device(""),
    m_fps(1),
    m_thread(0)
{
    m_restore = output();
}

void QCamCapture::setDevice(const QString &device){
    if ( m_device != device ){
        m_device = device;
        if ( m_device  != "" ){

            QStateContainer<QCamCaptureThread>& stateCont =
                    QStateContainer<QCamCaptureThread>::instance(this);

            m_thread = stateCont.state(m_device);
            if ( m_thread == 0 ){
                m_thread = new QCamCaptureThread(device);
                stateCont.registerState(m_device, m_thread);
            }

            setOutput(m_thread->output());
            connect( m_thread, SIGNAL(inactiveMatChanged()), this, SLOT(switchMat()));

            if ( m_thread->isCaptureOpened() ){
                setImplicitWidth (m_thread->captureWidth());
                setImplicitHeight(m_thread->captureHeight());

                if ( !m_thread->paused() ){
                    if ( m_thread->timer()->isActive() ){
                        if ( m_thread->timer()->interval() != (1000 / m_fps) ){
                            m_thread->timer()->stop();
                            m_thread->timer()->start(1000 / m_fps);
                        }
                    } else {
                        m_thread->timer()->start(1000 / m_fps);
                    }
                }

            } else
                qDebug() << "Open CV error : Could not open capture : " << m_device;
        }
        emit deviceChanged();
    }
}

void QCamCapture::setFps(qreal fps){
    if ( fps != m_fps ){
        m_fps = fps;
        if ( m_thread ){
            if ( !m_thread->paused() && m_thread->isCaptureOpened() ){
                if ( m_thread->timer()->isActive() )
                    m_thread->timer()->stop();
                m_thread->timer()->start(1000 / m_fps);
            }
        }
        emit fpsChanged();
    }
}

void QCamCapture::switchMat(){
    if ( m_thread ){
        setOutput(m_thread->output());
        m_thread->processNextFrame();
        update();
	}
}

void QCamCapture::setPaused(bool paused){
    if ( m_thread->paused() != paused ){
        m_thread->setPaused(paused);
        if ( paused ){
            m_thread->timer()->stop();
        } else
            m_thread->timer()->start(1000 / m_fps);
        emit pausedChanged();
    }
}

bool QCamCapture::paused() const{
    return m_thread->paused();
}

QCamCapture::~QCamCapture(){
    QStateContainer<QCamCaptureThread>& stateCont =
            QStateContainer<QCamCaptureThread>::instance(this);
    m_thread = stateCont.state(m_device);
    if (m_thread != 0){
        disconnect( m_thread, SIGNAL(inactiveMatChanged()), this, SLOT(switchMat()));
    }
    setOutput(m_restore);
}