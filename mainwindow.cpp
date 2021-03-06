#include "mainwindow.h"
#include "ui_mainwindow.h"

#ifdef Q_OS_LINUX
QString *OS=new QString("Linux");
#endif
#ifdef Q_OS_WIN32
QString *OS=new QString("Windows");
#include <windows.h>
#endif

MainWindow::MainWindow(QWidget *parent) :
    QMainWindow(parent),
    ui(new Ui::MainWindow)
{    


    ui->setupUi(this);

    createActions();
    createTrayIcon () ;
    setIcon();

    ui->menubar->setVisible(true);

    /*if(*OS == "Windows")
    {
        ui->taskManagerAction->setEnabled(true);
    }*/

    trayIcon->show();

    linuxPass = QString() ;

    time = QDateTime::currentDateTime();

    bSdEnabled = false ;

    iShowedSeconds = 0 ;
    iShowedMinutes = 0 ;
    iShowedHours = 0 ;

    loadNotes();

    timer = new QTimer (this) ;
    connect (timer, SIGNAL (timeout()), this, SLOT (updateTime())) ;
    timer->start (1000) ;
}
MainWindow::~MainWindow()
{   
    delete ui;
	ui = NULL;
    delete trayIcon;
	trayIcon = NULL;
    delete trayIconMenu;
	trayIconMenu = NULL;
    delete open;
	open = NULL;
	delete timer;
	timer = NULL;


}

void MainWindow::lineChanged()
{
    ui->okButton->setEnabled(ui->sLineEdit->hasAcceptableInput() &&
                         ui->mLineEdit->hasAcceptableInput() &&
                         ui->hLineEdit->hasAcceptableInput());
}

void MainWindow::on_sLineEdit_editingFinished()
{
    int iSecondsBef ;
    QString strSeconds ;
    iSecondsBef = GetIntVal(ui->sLineEdit->text().toStdString()) ;
    iShowedSeconds = iSecondsBef % 60 ;
    iSecondsBef = iSecondsBef / 60 ;
    strSeconds = IntToString(iShowedSeconds).c_str() ;
    ClearAndAdd (ui->sLineEdit, strSeconds) ;

    if (iSecondsBef > 0)
    {
        int iMinutesBef ;
        QString strMinutes ;
        iMinutesBef = iShowedMinutes + iSecondsBef;
        iShowedMinutes = iMinutesBef % 60 ;
        iMinutesBef = iMinutesBef / 60 ;
        strMinutes = IntToString (iShowedMinutes).c_str() ;
        ClearAndAdd(ui->mLineEdit, strMinutes);

        if(iMinutesBef > 0)
        {
            QString strHours ;
            iShowedHours = iShowedHours + iMinutesBef ;
            strHours = IntToString (iShowedHours).c_str() ;
            ClearAndAdd(ui->hLineEdit, strHours);
        }
    }

}

void MainWindow::on_mLineEdit_editingFinished()
{
    int iMinutesBef ;
    QString strMinutes ;
    iMinutesBef = GetIntVal (ui->mLineEdit->text().toStdString()) ;
    iShowedMinutes = iMinutesBef % 60 ;
    iMinutesBef = iMinutesBef / 60 ;
    strMinutes = IntToString (iShowedMinutes).c_str() ;
    ClearAndAdd(ui->mLineEdit, strMinutes);

    if (iMinutesBef > 0)
    {
        QString strHours ;
        iShowedHours = iShowedHours + iMinutesBef ;
        strHours = IntToString(iShowedHours).c_str() ;
        ClearAndAdd(ui->hLineEdit, strHours);
    }
}

void MainWindow::on_hLineEdit_editingFinished()
{
    QString strHours ;
    iShowedHours = GetIntVal (ui->hLineEdit->text().toStdString()) ;
    strHours = IntToString(iShowedHours).c_str() ;
    ClearAndAdd(ui->hLineEdit, strHours);
}


void MainWindow::on_abButton_clicked()
{
    if (setPassword.isEmpty())
    {

        time = QDateTime::currentDateTime();

        bSdEnabled = false ;

        displayLcd() ;
    }
    else if (reqPassword())
    {

        time = QDateTime::currentDateTime();

        bSdEnabled = false ;

        displayLcd() ;

    }
}

void MainWindow::displayLcd ()
{
    ui->lcdHours->display(QString::number(QDateTime::currentDateTime().secsTo(time)/3600));
    ui->lcdMinutes->display(QString::number((QDateTime::currentDateTime().secsTo(time) / 60) % 60));
    ui->lcdSeconds->display(QString::number(QDateTime::currentDateTime().secsTo(time) % 60));

}

void MainWindow::on_exitButton_clicked()
{
    toQuit() ;
}

void MainWindow::on_okButton_clicked()
{
    if (setPassword.isEmpty())
    {
        passSet() ;
    }
    else
    {
        if (reqPassword())
        {
            passSet() ;
        }
    }
}

void MainWindow::passSet()
{
    if (*OS == "Windows")
        passSetup();

    else if (*OS == "Linux")
    {
        //modify to ask for user password
        reqLnxPass();
        if (linuxPass.isNull()) QMessageBox::warning (this, tr("Warning"),
                                                     tr("Linux password was not set"), QMessageBox::Ok);
        else
            passSetup();
    }
    else
    {
        QMessageBox::critical (this, tr("Error"),
                               tr("OS not supoorted"), QMessageBox::Ok) ;
        bSdEnabled = false ;
        qApp->quit();
    }
}

void MainWindow::reqLnxPass()
{
    getLinuxPass *askPas = new getLinuxPass (this, &linuxPass) ;

    askPas->exec();
}

void MainWindow::passSetup()
{
    time = QDateTime::currentDateTime() ;
    time = time.addSecs(GetIntVal(ui->hLineEdit->text().toStdString()) * 3600 + GetIntVal(ui->mLineEdit->text().toStdString()) * 60 + GetIntVal(ui->sLineEdit->text().toStdString()));

    bSdEnabled = true ;

}

void MainWindow::updateTime()
{

    if (bSdEnabled)
    {

        if (!(time > QDateTime::currentDateTime()))
        {
            if (*OS == "Windows")
            {
                system ("shutdown -s -f -t 0") ;
                bSdEnabled = false ;
            }
            else if (*OS == "Linux")
            {
                QString strCommand ;
                strCommand = "echo " + linuxPass + " | sudo -S shutdown -P now" ;
                system (strCommand.toStdString().c_str());
                bSdEnabled = false ;
                linuxPass = QString() ;

            }
            else
                {
                    QMessageBox::critical (this, tr("Error"),
                                           tr("OS not supoorted"), QMessageBox::Ok) ;
                    bSdEnabled = false ;
                    qApp->quit();
                }
        }
        else
        {

            displayLcd() ;

        }
    }

    for (std::map<std::string, Note>::iterator iter = Notes.begin(); iter != Notes.end();)
    {

        if (iter->second.timeExpired(time))
        {
            if (iter->second.getTrayDisplay() == true)
            {
                if (iter->second.isSound())
                    iter->second.playWav();
                trayIcon->showMessage(QString::fromUtf8(iter->second.getTitle().toStdString().c_str()),
                                      QString::fromUtf8(iter->second.getDetails().toStdString().c_str())) ;
            }
            if (iter->second.getWindowDisplay() == true)
            {
                if(isHidden())
                    show() ;
                iter->second.displayNote() ;
            }
            Notes.erase(iter++) ;
        }
        else ++iter ;
    }
}

std::string MainWindow::IntToString(int IntValue)
{
    char *MyBuff ;
    std::string strRetVal ;

    MyBuff = new char[100] ;

    memset (MyBuff, '\0', 100) ;
    _snprintf(MyBuff, sizeof(MyBuff), "%d", IntValue);
    //itoa (IntValue, MyBuff, 10) ;

    strRetVal = MyBuff ;

    delete [] MyBuff ;

    return (strRetVal) ;
}

int MainWindow::GetIntVal(std::string StrConvert)
{
    int intReturn ;

    intReturn = atoi (StrConvert.c_str()) ;

    return (intReturn) ;
}

void MainWindow::ClearAndAdd(QLineEdit* lineEdit, QString &text)
{
    lineEdit->clear() ;
    lineEdit->insert(text);
}

void MainWindow::on_aboutQtAction_triggered()
{
    qApp->aboutQt() ;
}

void MainWindow::on_exitAction_triggered()
{
    toQuit() ;
}

void MainWindow::on_resetAction_triggered()
{
    iShowedSeconds = 0 ;
    iShowedMinutes = 0 ;
    iShowedHours = 0 ;

    ui->sLineEdit->setText("0");
    ui->mLineEdit->setText("0");
    ui->hLineEdit->setText("0");
}

void MainWindow::on_aboutCptAction_triggered()
{
    QMessageBox::about(this, tr("About Captain Shutdown"),
                       tr("<h2>Captain Shutdown 3.0</h2>"
                       "<p>Created by Radu Daniel Alexandru"
                       "<p>danyhk94@gmail.com"
                        "<p><a href = http://tinyprojectz.blogspot.com>My Blog</a>"
                       "<p>Captain Shutdown is a free application "
                       "that allows time scheduling of computer shutdown. "
                       "Currently, it works for Windows and Linux."
                          "<p><a href = http://www.softpedia.com/progClean/Captain-Shutdown-Clean-217944.html>Softpedia 100% Clean Award</a>")) ;
}

void MainWindow::on_setPAction_triggered()
{
    SetPassword *setPas = new SetPassword(this) ;

    setPas->show() ;
}

void MainWindow::setPAction_correct()
{

    QMessageBox::information (this, tr("Success"),
                              tr("Password successfully set"), QMessageBox::Ok) ;
    ui->setPAction->setEnabled(false) ;
    ui->editPAction->setEnabled(true) ;
    ui->removePAction->setEnabled(true) ;
}

void MainWindow::setPAction_incorrect ()
{
    setPassword.clear();
}

void MainWindow::setPAction_register(const QString &pass)
{
    setPassword = pass ;
}

void MainWindow::closeEvent(QCloseEvent *event)
{
    if(trayIcon->isVisible())
    {
        trayIcon->showMessage(tr("Hey ... I think it moved !!!"),
                              tr("This application is still running. To quit please click the Exit button.")) ;
        hide() ;
        event->ignore();
    }
}



bool MainWindow::reqPassword()
{
    bReqPassword = false ;

    AskPassword *askPas = new AskPassword (this, setPassword) ;

    askPas->exec();

    return bReqPassword ;
}

void MainWindow::slAskAccepted()
{
    bReqPassword = true ;
}

void MainWindow::saveNotes()
{
    ofstream myfile("notes.txt");

    myfile << Notes.size() << endl;

    for (std::map<std::string, Note>::iterator iter = Notes.begin(); iter != Notes.end();)
    {
        myfile << iter->second;
        iter++;
    }
}

void MainWindow::loadNotes()
{
    std::ifstream myfile("notes.txt");
    Note toLoad;
    int size;

    myfile >> size;

    for (int i = 0; i < size; i++)
    {
        myfile >> toLoad;

        if (!(toLoad.getTime() > QDateTime::currentDateTime()))
        {
            continue;
        }

        //QMessageBox::about(this, "da", QString::number(toLoad.isSound()) + QString::number(toLoad.getTrayDisplay()) + QString::number(toLoad.getWindowDisplay()));

        Notes.insert( make_pair(toLoad.getTitle().toStdString(), toLoad) ) ;
    }
}

void MainWindow::slAskRejcted()
{
    bReqPassword = false ;
}

void MainWindow::on_editPAction_triggered()
{
    if (reqPassword())
    {
        on_setPAction_triggered() ;
    }
}

void MainWindow::on_removePAction_triggered()
{
    if (reqPassword())
    {
        setPassword.clear();
        ui->setPAction->setEnabled(true) ;
        ui->editPAction->setEnabled(false) ;
        ui->removePAction->setEnabled(false) ;
    }
}

bool MainWindow::getSdState()
{
    return bSdEnabled ;
}


void MainWindow::on_addNoteAction_triggered()
{
    if(setPassword.isEmpty())
    {
        AddNote *addNote = new AddNote (this, &Notes, bSdEnabled) ;
        addNote->show();
    }
    else
        if (reqPassword())
        {
            AddNote *addNote = new AddNote (this, &Notes, bSdEnabled) ;
            addNote->show();
        }
}

void MainWindow::toQuit()
{
    if (setPassword.isEmpty())
    {
        saveNotes();
        qApp->quit();
    }
    else
        if (reqPassword())
        {
            saveNotes();
            qApp->quit();
        }

}

void MainWindow::createActions()
{
    open = new QAction (tr("&Show"), this) ;
    connect (open, SIGNAL (triggered()), this , SLOT (show())) ;

    exit = new QAction (tr("&Exit"), this) ;
    connect (exit, SIGNAL (triggered()), this, SLOT (toQuit())) ;
}

void MainWindow ::createTrayIcon()
{
    trayIconMenu = new QMenu(this) ;

    trayIconMenu->addAction(open) ;
    trayIconMenu->addAction(exit) ;

    trayIcon = new QSystemTrayIcon(this) ;
    trayIcon->setContextMenu(trayIconMenu) ;

    connect (trayIcon, SIGNAL (activated(QSystemTrayIcon::ActivationReason)),
             this, SLOT (trayIconClicked(QSystemTrayIcon::ActivationReason))) ;
}

void MainWindow::setIcon()
{
    trayIcon->setIcon(QIcon("shutdown.png")) ;
}

void MainWindow::trayIconClicked(QSystemTrayIcon::ActivationReason reason)
{
    if (reason == QSystemTrayIcon::Trigger)
    {
        if(!bSdEnabled)
            trayIcon->showMessage(tr("Timer off"), tr("Shutdown timer hasn't been activated")) ;
        else
        {
            strTimer = IntToString (QDateTime::currentDateTime().secsTo(time) / 3600) + " : " + IntToString((QDateTime::currentDateTime().secsTo(time) / 60) % 60) + " : " + IntToString (QDateTime::currentDateTime().secsTo(time) % 60) ;
            trayIcon->showMessage (tr("It's a bird, it's a plane, it's ..."), strTimer.c_str()) ;
        }

    }
}

void MainWindow::on_sLineEdit_focussed(bool focus)
{
    if (focus)
        ui->sLineEdit->clear();
    else
        if (ui->sLineEdit->text().isEmpty())
        {
            ui->sLineEdit->setText(QString::fromUtf8("0")) ;
            iShowedSeconds = 0 ;
        }
}

void MainWindow::on_mLineEdit_focussed(bool focus)
{
    if (focus)
        ui->mLineEdit->clear();
    else
        if (ui->mLineEdit->text().isEmpty())
        {
            ui->mLineEdit->setText(QString::fromUtf8("0")) ;
            iShowedMinutes = 0 ;
        }
}

void MainWindow::on_hLineEdit_focussed(bool focus)
{
    if (focus)
        ui->hLineEdit->clear();
    else
        if (ui->hLineEdit->text().isEmpty())
        {
            ui->hLineEdit->setText(QString::fromUtf8("0")) ;
            iShowedHours = 0 ;
        }
}

void MainWindow::on_changeSound_triggered()
{
    QMessageBox::about(this, tr("How to change sound"), tr("You can change the sound created by notes that pop by chaning the \"bell.wav\" file to another one with the same name")) ;
}
