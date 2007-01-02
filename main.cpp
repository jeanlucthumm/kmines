/*
 * Copyright (c) 1996-2004 Nicolas HADACEK (hadacek@kde.org)
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.

 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.

 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
 */

#include "main.h"
#include "main.moc"

#include <QFocusEvent>


#include <kapplication.h>
#include <klocale.h>
#include <kcmdlineargs.h>
#include <kaboutdata.h>
#include <kstandardaction.h>
#include <kkeydialog.h>
#include <kstandardgameaction.h>
#include <kaction.h>
#include <kactioncollection.h>
#include <knotifyconfigwidget.h>
#include <ktoggleaction.h>
#include <kdebug.h>
#include <khighscore.h>
#include <kconfigdialog.h>
#include <kicon.h>

#include "settings.h"
#include "status.h"
#include "highscores.h"
#include "version.h"
#include "dialogs.h"

const MainWidget::KeyData MainWidget::KEY_DATA[NB_KEYS] = {
{I18N_NOOP("Move Up"),     "keyboard_moveup",    Qt::Key_Up,    SLOT(moveUp())},
{I18N_NOOP("Move Down"),   "keyboard_movedown",  Qt::Key_Down,  SLOT(moveDown())},
{I18N_NOOP("Move Right"),  "keyboard_moveright", Qt::Key_Right, SLOT(moveRight())},
{I18N_NOOP("Move Left"),   "keyboard_moveleft",  Qt::Key_Left,  SLOT(moveLeft())},
{I18N_NOOP("Move at Left Edge"), "keyboard_leftedge", Qt::Key_Home, SLOT(moveLeftEdge())},
{I18N_NOOP("Move at Right Edge"), "keyboard_rightedge", Qt::Key_End, SLOT(moveRightEdge())},
{I18N_NOOP("Move at Top Edge"), "keyboard_topedge", Qt::Key_PageUp, SLOT(moveTop())},
{I18N_NOOP("Move at Bottom Edge"), "keyboard_bottomedge", Qt::Key_PageDown, SLOT(moveBottom())},
{I18N_NOOP("Reveal Mine"), "keyboard_revealmine", Qt::Key_Space, SLOT(reveal())},
{I18N_NOOP("Mark Mine"),   "keyboard_markmine",  Qt::Key_W,     SLOT(mark())},
{I18N_NOOP("Automatic Reveal"), "keyboard_autoreveal", Qt::Key_Return, SLOT(autoReveal())}
};


MainWidget::MainWidget()
  : KZoomMainWindow(4, 100, 1)
{

    _status = new Status(this);
    connect(_status, SIGNAL(gameStateChangedSignal(KMines::GameState)),
            SLOT(gameStateChanged(KMines::GameState)));
    connect(_status, SIGNAL(pause()), SLOT(pause()));

    QAction *action;

	// Game & Popup
    action = KStandardGameAction::gameNew(_status, SLOT(restartGame()), this);
    actionCollection()->addAction(action->objectName(), action);
    _pause = KStandardGameAction::pause(_status, SLOT(pauseGame()), this);
    actionCollection()->addAction(_pause->objectName(), _pause);
    action = KStandardGameAction::highscores(this, SLOT(showHighscores()), this);
    actionCollection()->addAction(action->objectName(), action);
    action = KStandardGameAction::quit(qApp, SLOT(quit()), this);
    actionCollection()->addAction(action->objectName(), action);

	// keyboard
    _keybCollection = new KActionCollection(static_cast<QWidget*>(this));
    for (uint i=0; i<NB_KEYS; i++) {
        const KeyData &d = KEY_DATA[i];
        QAction *action = _keybCollection->addAction(d.name);
        action->setText(i18n(d.label));
        connect(action, SIGNAL(triggered(bool) ), _status, d.slot);
        action->setShortcut(d.keycode);
        addAction(action);
    }

	// Settings
    action = KStandardAction::preferences(this, SLOT(configureSettings()), this);
    actionCollection()->addAction(action->objectName(), action);
    action = KStandardAction::keyBindings(this, SLOT(configureKeys()), this);
    actionCollection()->addAction(action->objectName(), action);
    action = KStandardAction::configureNotifications(this, SLOT(configureNotifications()), this);
    actionCollection()->addAction(action->objectName(), action);
    action = KStandardGameAction::configureHighscores(this, SLOT(configureHighscores()), this);
    actionCollection()->addAction(action->objectName(), action);

	// Levels
    _levels = KStandardGameAction::chooseGameType(_status, SLOT(newGame(int)), actionCollection());
    actionCollection()->addAction(_levels->objectName(), _levels);
    QStringList list;
    for (uint i=0; i<=Level::NB_TYPES; i++)
        list += i18n(Level::LABELS[i]);
    _levels->setItems(list);

    // Adviser
    _advise =
        KStandardGameAction::hint(_status, SLOT(advise()), actionCollection());
    _solve = KStandardGameAction::solve(_status, SLOT(solve()), actionCollection());
    action = actionCollection()->addAction("solve_rate");
    action->setText(i18n("Solving Rate..."));
    connect(action, SIGNAL(triggered(bool) ), _status, SLOT(solveRate()));

    // Log
    QAction* logAct = actionCollection()->addAction( "log_view" );
    logAct->setIcon( KIcon("viewmag") );
    logAct->setText( i18n("View Log") );
    connect(logAct, SIGNAL(triggered(bool)), _status, SLOT(viewLog()));

    QAction* replayAct = actionCollection()->addAction( "log_replay" );
    replayAct->setIcon( KIcon("player_play") );
    replayAct->setText( i18n("Replay Log") );
    connect(replayAct, SIGNAL(triggered(bool)), _status, SLOT(replayLog()));

    QAction* saveAct = actionCollection()->addAction( "log_save" );
    saveAct->setIcon( KIcon("filesave") );
    saveAct->setText( i18n("Save Log...") );
    connect(saveAct, SIGNAL(triggered(bool)), _status, SLOT(saveLog()));

    QAction* loadAct = actionCollection()->addAction( "log_load" );
    loadAct->setIcon( KIcon("fileopen") );
    loadAct->setText( i18n("Load Log...") );
    connect(loadAct, SIGNAL(triggered(bool)), _status, SLOT(loadLog()));

    setupGUI( KMainWindow::Save | Create );
    readSettings();
    setCentralWidget(_status);
    init("popup");
    // we want to receive RMB event's on field rather than context menu
    _status->field()->setContextMenuPolicy(Qt::PreventContextMenu);
    addZoomable(_status->field());
}

bool MainWidget::queryExit()
{
    _status->checkBlackMark();
    return KZoomMainWindow::queryExit();
}

void MainWidget::readSettings()
{
    settingsChanged();
    Level::Type type = (Level::Type) Settings::level();
    _levels->setCurrentItem(type);
    _status->newGame(type);
}

void MainWidget::showHighscores()
{
    KExtHighscore::show(this);
}

void MainWidget::focusOutEvent(QFocusEvent *e)
{
    if ( Settings::pauseFocus() && e->reason()==Qt::ActiveWindowFocusReason
          && _status->isPlaying() ) pause();
    KMainWindow::focusOutEvent(e);
}

void MainWidget::configureSettings()
{
    if ( KConfigDialog::showDialog("settings") ) return;

    KConfigDialog *dialog = new KConfigDialog(this, "settings", Settings::self());
    GameConfig *gc = new GameConfig;
    dialog->addPage(gc, i18n("Game"), "package_system");
    dialog->addPage(new AppearanceConfig, i18n("Appearance"), "style");
    CustomConfig *cc = new CustomConfig;
    dialog->addPage(cc, i18n("Custom Game"), "package_settings");
    connect(dialog, SIGNAL(settingsChanged(const QString &)), SLOT(settingsChanged()));
    dialog->show();
    cc->init();
    gc->init();
}

void MainWidget::configureHighscores()
{
    KExtHighscore::configure(this);
}

void MainWidget::settingsChanged()
{
    bool enabled = Settings::keyboardGame();
    QList<QAction *> list = _keybCollection->actions();
    QList<QAction *>::Iterator it;
    for (it = list.begin(); it!=list.end(); ++it)
        (*it)->setEnabled(enabled);
    _status->settingsChanged();
}

void MainWidget::configureKeys()
{
    KKeyDialog d(KKeyChooser::AllActions, KKeyChooser::LetterShortcutsAllowed, this);
    d.insert(_keybCollection, i18n("Keyboard game"));
    d.insert(actionCollection(), i18n("General"));
    d.configure();
}

void MainWidget::configureNotifications()
{
    KNotifyConfigWidget::configure(this);
}

void MainWidget::gameStateChanged(KMines::GameState state)
{
    stateChanged(KMines::STATES[state]);
    if ( state==Playing ) setFocus();
}

void MainWidget::pause()
{
    _pause->trigger();
}

void MainWidget::writeZoomSetting(uint zoom)
{
  Settings::setCaseSize(zoom);
  Settings::writeConfig();
}

uint MainWidget::readZoomSetting() const
{
  return Settings::caseSize();
}

void MainWidget::writeMenubarVisibleSetting(bool visible)
{
  Settings::setMenubarVisible(visible);
  Settings::writeConfig();
}

bool MainWidget::menubarVisibleSetting() const
{
  return Settings::menubarVisible();
}

//----------------------------------------------------------------------------
static const char *DESCRIPTION
    = I18N_NOOP("KMines is a classic mine sweeper game");

int main(int argc, char **argv)
{
    KHighscore::init("kmines");

    KAboutData aboutData("kmines", I18N_NOOP("KMines"), LONG_VERSION,
						 DESCRIPTION, KAboutData::License_GPL,
						 COPYLEFT, 0, HOMEPAGE);
    aboutData.addAuthor("Nicolas Hadacek", 0, EMAIL);
	aboutData.addCredit("Andreas Zehender", I18N_NOOP("Smiley pixmaps"));
    aboutData.addCredit("Mikhail Kourinny", I18N_NOOP("Solver/Adviser"));
    aboutData.addCredit("Thomas Capricelli", I18N_NOOP("Magic reveal mode"));
    KCmdLineArgs::init(argc, argv, &aboutData);

    KApplication a;
    KGlobal::locale()->insertCatalog("libkdegames");
    KExtHighscore::ExtManager manager;

    if ( a.isSessionRestored() ) RESTORE(MainWidget)
    else {
        MainWidget *mw = new MainWidget;
        mw->show();
    }
    return a.exec();
}
