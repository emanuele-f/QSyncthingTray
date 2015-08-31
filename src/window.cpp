/****************************************************************************
**
** Copyright (C) 2015 The Qt Company Ltd.
** Contact: http://www.qt.io/licensing/
**
** This file is part of the examples of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:BSD$
** You may use this file under the terms of the BSD license as follows:
**
** "Redistribution and use in source and binary forms, with or without
** modification, are permitted provided that the following conditions are
** met:
**   * Redistributions of source code must retain the above copyright
**     notice, this list of conditions and the following disclaimer.
**   * Redistributions in binary form must reproduce the above copyright
**     notice, this list of conditions and the following disclaimer in
**     the documentation and/or other materials provided with the
**     distribution.
**   * Neither the name of The Qt Company Ltd nor the names of its
**     contributors may be used to endorse or promote products derived
**     from this software without specific prior written permission.
**
**
** THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
** "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
** LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
** A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
** OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
** SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
** LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
** DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
** THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
** (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
** OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE."
**
** $QT_END_LICENSE$
**
****************************************************************************/

#include "window.h"

#ifndef QT_NO_SYSTEMTRAYICON

#include <QtGui>

#include <QAction>
#include <QCheckBox>
#include <QComboBox>
#include <QGroupBox>
#include <QLabel>
#include <QLineEdit>
#include <QMenu>
#include <QPushButton>
#include <QSpinBox>
#include <QTextEdit>
#include <QVBoxLayout>
#include <QMessageBox>
#include <iostream>

//! [0]
Window::Window()
  : mSyncConnector(new mfk::connector::SyncConnector(QUrl(tr("http://127.0.0.1:8384"))))
  , settings("sieren", "QSyncThingTray")
{
    loadSettings();
    createSettingsGroupBox();

    createActions();
    createTrayIcon();

    connect(testConnection, SIGNAL(clicked()), this, SLOT(testURL()));
    connect(syncThingUrl, SIGNAL(currentIndexChanged(int)), this, SLOT(setIcon(int)));
    connect(trayIcon, SIGNAL(messageClicked()), this, SLOT(messageClicked()));
    connect(trayIcon, SIGNAL(activated(QSystemTrayIcon::ActivationReason)),
            this, SLOT(iconActivated(QSystemTrayIcon::ActivationReason)));
    connect(authCheckBox, SIGNAL(stateChanged(int)), this, SLOT(authCheckBoxChanged(int)));
    connect(filePathBrowse, SIGNAL(clicked()), this, SLOT(showFileBrowser()));
  
    QVBoxLayout *mainLayout = new QVBoxLayout;
    mainLayout->addWidget(settingsGroupBox);
    mainLayout->addWidget(filePathGroupBox);
    setLayout(mainLayout);
    testURL();
    mSyncConnector->setConnectionHealthCallback(std::bind(&Window::updateConnectionHealth, this,
                                                        std::placeholders::_1));
    mSyncConnector->setProcessSpawnedCallback([&](bool success)
      {
        if (success)
        {
          appSpawnedLabel->setText(tr("Launched"));
        }
        else
        {
          appSpawnedLabel->setText(tr("Not started"));
        }

      });
    mSyncConnector->spawnSyncThingProcess(mCurrentSyncThingPath);

    setIcon(0);
    trayIcon->show();
    #ifdef Q_OS_MAC
      this->setWindowIcon(QIcon(":/images/syncthing.icns"));
    #endif
    setWindowTitle(tr("QSyncThingTray"));
    resize(400, 300);
    setSizePolicy(QSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed));
}


void Window::setVisible(bool visible)
{
  QDialog::setVisible(visible);
  raise();
}


void Window::closeEvent(QCloseEvent *event)
{
    if (trayIcon->isVisible())
    {
        hide();
        event->ignore();
    }
}


void Window::setIcon(int index)
{
  QIcon icon;
  switch(index)
  {
    case 0:
      icon = QIcon(":/images/syncthingBlue.png");
      break;
    case 1:
      icon = QIcon(":/images/syncthingGrey.png");
      break;
    default:
      icon = QIcon(":/images/syncthingGrey.png");
      break;
  }
  trayIcon->setIcon(icon);
  setWindowIcon(icon);

  trayIcon->setToolTip("SyncThing");
}


void Window::testURL()
{
  mCurrentUrl = QUrl(syncThingUrl->text());
  mCurrentUserName = userName->text().toStdString();
  mCurrentUserPassword = userPassword->text().toStdString();
  mSyncConnector->setURL(QUrl(syncThingUrl->text()), mCurrentUserName, mCurrentUserPassword,
                         [&](std::string result, bool success) {
    if (success)
    {
      urlTestResultLabel->setText(tr("Connected"));
      connectedState->setText(tr("Connected"));
      setIcon(0);
    }
    else
    {
      urlTestResultLabel->setText(tr("Error: ") + result.c_str());
      setIcon(1);
    }
  });
  saveSettings();
}


void Window::updateConnectionHealth(std::map<std::string, std::string> status)
{
  if (status.at("state") == "1")
  {
    std::string connectionNumber = status.at("connections");
    numberOfConnectionsAction->setVisible(true);
    numberOfConnectionsAction->setText(tr("Connections: ") + connectionNumber.c_str());
    connectedState->setText(tr("Connected"));
    setIcon(0);
  }
  else
  {
    connectedState->setText(tr("Not Connected"));
    showMessage("Not Connected", "Could not find SyncThing.");
    setIcon(1);
  }
}


void Window::iconActivated(QSystemTrayIcon::ActivationReason reason)
{
  switch (reason)
  {
  case QSystemTrayIcon::Trigger:
  case QSystemTrayIcon::DoubleClick:
      break;
  case QSystemTrayIcon::MiddleClick:
      break;
  default:
      ;
  }
  setIcon(1);
}


void Window::showWebView()
{
  mSyncConnector->showWebView();
}


void Window::authCheckBoxChanged(int state)
{
  if (state)
  {
    showAuthentication(true);
  }
  else
  {
    showAuthentication(false);
  }
}


void Window::showMessage(std::string title, std::string body)
{
  trayIcon->showMessage(tr(title.c_str()), tr(body.c_str()), QSystemTrayIcon::Warning,
                        1000);
}


void Window::showFileBrowser()
{
  QString filename = QFileDialog::getOpenFileName(this,
                                          tr("Open SyncThing"), "", tr(""));
  mCurrentSyncThingPath = filename.toStdString();
  filePathLine->setText(filename);
  saveSettings();
  spawnSyncThingApp();
}


void Window::spawnSyncThingApp()
{
  mSyncConnector->spawnSyncThingProcess(mCurrentSyncThingPath);
}


void Window::messageClicked()
{
  QDialog::setVisible(true);
  raise();
}


void Window::createSettingsGroupBox()
{
  settingsGroupBox = new QGroupBox(tr("SyncThing URL"));

  iconLabel = new QLabel("URL");

  syncThingUrl = new QLineEdit(mCurrentUrl.toString());

  testConnection = new QPushButton(tr("Connect"));

  authCheckBox = new QCheckBox(tr("Authentication"), this);

  userNameLabel = new QLabel("User");
  userPasswordLabel = new QLabel("Password");

  userName = new QLineEdit(mCurrentUserName.c_str());
  userPassword = new QLineEdit(mCurrentUserPassword.c_str());
  userPassword->setEchoMode(QLineEdit::Password);

  urlTestResultLabel = new QLabel("Not Tested");

  QGridLayout *iconLayout = new QGridLayout;
  iconLayout->addWidget(iconLabel, 0, 0);
  iconLayout->addWidget(syncThingUrl,1, 0, 1, 14);
  iconLayout->addWidget(authCheckBox, 2, 0, 1, 2);
  iconLayout->addWidget(userNameLabel, 3, 0, 1, 2);
  iconLayout->addWidget(userPasswordLabel, 3, 2, 1 ,4);
  iconLayout->addWidget(userName, 4, 0, 1, 2);
  iconLayout->addWidget(userPassword, 4, 2, 1, 2 );
  iconLayout->addWidget(testConnection,5, 0, 1, 1);
  iconLayout->addWidget(urlTestResultLabel, 5, 1, 1, 4);
  settingsGroupBox->setLayout(iconLayout);
  settingsGroupBox->setMinimumWidth(400);
  settingsGroupBox->setSizePolicy(QSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed));

  filePathGroupBox = new QGroupBox(tr("SyncThing Application"));

  filePathLabel = new QLabel("Path");

  filePathLine = new QLineEdit(mCurrentSyncThingPath.c_str());

  filePathBrowse = new QPushButton(tr("Browse"));

  appSpawnedLabel = new QLabel(tr("Not started"));

  authCheckBox->setCheckState(Qt::Checked);
  if (mCurrentUserName.length() == 0)
  {
    showAuthentication(false);
    authCheckBox->setCheckState(Qt::Unchecked);
  }

  QGridLayout *filePathLayout = new QGridLayout;
  filePathLayout->addWidget(filePathLabel, 0, 0);
  filePathLayout->addWidget(filePathLine,1, 0, 1, 14);
  filePathLayout->addWidget(filePathBrowse,2, 0, 1, 1);
  filePathLayout->addWidget(appSpawnedLabel, 2, 1, 1, 1);

  filePathGroupBox->setLayout(filePathLayout);
  filePathGroupBox->setMinimumWidth(400);
  filePathGroupBox->setSizePolicy(QSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed));
}

void Window::createActions()
{
  connectedState = new QAction(tr("Not Connected"), this);
  connectedState->setDisabled(true);

  numberOfConnectionsAction = new QAction(tr("Connections: 0"), this);
  numberOfConnectionsAction->setDisabled(true);

  showWebViewAction = new QAction(tr("Open SyncThing"), this);
  connect(showWebViewAction, SIGNAL(triggered()), this, SLOT(showWebView()));
      
  preferencesAction = new QAction(tr("Preferences"), this);
  connect(preferencesAction, SIGNAL(triggered()), this, SLOT(showNormal()));
  
  showGitHubAction = new QAction(tr("Help"), this);
  connect(showGitHubAction, SIGNAL(triggered()), this, SLOT(showGitPage()));
  

  quitAction = new QAction(tr("&Quit"), this);
  connect(quitAction, SIGNAL(triggered()), qApp, SLOT(quit()));
}

void Window::createTrayIcon()
{
  trayIconMenu = new QMenu(this);
  trayIconMenu->addAction(connectedState);
  trayIconMenu->addAction(numberOfConnectionsAction);
  trayIconMenu->addSeparator();
  trayIconMenu->addAction(showWebViewAction);
  trayIconMenu->addAction(preferencesAction);
  trayIconMenu->addSeparator();
  trayIconMenu->addAction(showGitHubAction);
  trayIconMenu->addSeparator();
  trayIconMenu->addAction(quitAction);

  trayIcon = new QSystemTrayIcon(this);
  trayIcon->setContextMenu(trayIconMenu);
}

void Window::saveSettings()
{
  settings.setValue("url", mCurrentUrl.toString());
  settings.setValue("username", userName->text());
  settings.setValue("userpassword", userPassword->text());
  settings.setValue("syncthingpath", tr(mCurrentSyncThingPath.c_str()));
}

void Window::showAuthentication(bool show)
{
  if (show)
  {
    userName->show();
    userPassword->show();
    userNameLabel->show();
    userPasswordLabel->show();
  }
  else
  {
    userName->hide();
    userPassword->hide();
    userNameLabel->hide();
    userPasswordLabel->hide();
  }
}

void Window::loadSettings()
{
  mCurrentUrl.setUrl(settings.value("url").toString());
  if (mCurrentUrl.toString().length() == 0)
  {
    mCurrentUrl.setUrl(tr("http://127.0.0.1:8384"));
  }
  mCurrentUserPassword = settings.value("userpassword").toString().toStdString();
  mCurrentUserName = settings.value("username").toString().toStdString();
  mCurrentSyncThingPath = settings.value("syncthingpath").toString().toStdString();
}

void Window::showGitPage()
{
  QString link = "http://www.github.com/sieren/QSyncThingTray";
  QDesktopServices::openUrl(QUrl(link));
}
#endif