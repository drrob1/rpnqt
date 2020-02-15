/* Revision History
 * -------- -------
 * 9 Feb 20 -- Code finally works as intended, doing most of what the go versions do.  It only processes one command in the lineedit box at a time, ignoring other commands.
 *               I don't remember when I first decided to attempt this; I believe it was Sep-Oct 2019.  That's ~ 4 months ago.
 *
 *
 */

#include "mainwindow.h"
#include "ui_mainwindow.h"
#include <vector>
#include <QFile>
#include <QFileDialog>
#include <QDataStream>
#include <QMessageBox>
#include <QDialog>
#include <QInputDialog>
#include <QDir>

//#include <string>  already in macros.h
// ----------------------- my stuff
#include "macros.h"
#include "hpcalcc2.h"
// in hpcalcc2.h is this
//  struct calcPairType {
//      vector<string> ss;
//      double R;
//  };

#include "tokenizec2.h"
#include "timlibc2.h"
#include "holidaycalc2.h"
#include "getcommandline2.h"
#include "makesubst.h"

void ProcessInput(string cmdstr);  // forward reference

//double R;  not global
//int I;     not global
int SigFig = -1;  // qt creator complained about a double definition of sigfig, so this one is now SigFig
struct RegisterType {
    double value;
    QString name;  // w/ my struggles about string <--> QString, I have to more careful.  I have to use a QString for I/O.
};

ARRAYOF RegisterType Storage[36];  // var Storage []RegisterType  in Go syntax
ARRAYOF double Stk[StackSize];
bool CombinedFileExists;
const char *CombinedFileName = "RPNStackStorage.sav";
QString CombinedFilenamestring;
string LastCompiledDate = __DATE__;
string LastCompiledTime = __TIME__;

// Ui::MainWindow *ui;  //make ui global   It worked, but I ended up having this passes as a param.

enum OutputStateEnum {outputfix, outputfloat, outputgen};
OutputStateEnum OutputState = outputfix;

// constructor
MainWindow::MainWindow(QWidget *parent) : QMainWindow(parent), ui(new Ui::MainWindow) { // constructor
    ui->setupUi(this);

    CombinedFilenamestring = QDir::homePath() + "/" + CombinedFileName;
    QFile fh(CombinedFilenamestring);
    CombinedFileExists = fh.exists();
    if (CombinedFileExists) { // read the file into the stack and storage registers.
        bool opened = fh.open(QFile::ReadOnly);
        if (NOT opened) QMessageBox::warning(this," file not opened", "Stack and Storage combined input file could not be opened, but it exists.");
        QDataStream combinedfile(&fh);
        for (int i = 0; i < StackSize; i++) {  // read in Stk
          combinedfile >> Stk[i];
          PUSHX(Stk[i]);
        }

        for (int i = 0; i < 36; i++) {  // read in Storage registers
            QString qstr;
            double r;
            combinedfile >> r;
            combinedfile >> qstr;
            Storage[i].value = r;
            Storage[i].name = qstr;
        }
    } else {  // init the stack and storage registers to the zero value for each type.
        for (int i = 0; i < StackSize; i++) Stk[i] = 0.;  // Zero out stack

        for (int i = 0; i < 36; i++) {
            Storage[i].value = 0.;
            Storage[i].name = "";
        }
    }

    QStringList list = QCoreApplication::arguments();
    QString qstr, argv;
    if (NOT list.isEmpty()){
        QStringList::iterator iter;
        argv = "";
        for (iter = list.begin(); iter != list.end(); iter++){
            qstr = *iter;
            argv += qstr + " ";
        }
//        ui->lineEdit->setText(argv);  It's not doing what I want when run thru Qt Creator.
    }

}

MainWindow::~MainWindow() { // destructor
    delete ui;
}


void WriteStack(Ui::MainWindow *ui) {

    calcPairType calcpair; // var calcpair calcPairType  using Go syntax
    ARRAYOF QString qstack[StackSize];

    IF OutputState == outputfix THEN
            // _, stackslice = GetResult("DUMPFIXED");
            calcpair = GetResult("DUMPFIXED");
    ELSIF OutputState == outputfloat THEN
            // _, stackslice = GetResult("DUMPFLOAT");
            calcpair = GetResult("DUMPFLOAT");
    ELSIF OutputState == outputgen THEN
            // _, stackslice = GetResult("DUMP");
            calcpair = GetResult("DUMP");
    ENDIF;

    ui->listWidget_Stack->clear();
    // assign stack of qstrings and write the string vectors listWidget_Stack
    for (int i = 0; i < StackSize; i++) {
        qstack[i] = qstack[i].fromStdString(calcpair.ss[i]);
        ui->listWidget_Stack->addItem(qstack[i]);
    }
} // WriteStack()

int FUNCTION GetRegIdx(char c) {
    int idx = 0;
    char ch;

    ch = CAP(c);
    if ((ch >= '0') AND (ch <= '9')) {
        idx = ch - '0';
    } else if ((ch >= 'A') AND (ch <= 'Z'))   {
        idx = ch - 'A' + 10;
    }
    return idx;
} // GetRegIdx

char FUNCTION GetRegChar(int i) {
    char ch = '\0';

    if ((i >= 0) AND (i <= 9)) {
        ch = '0' + i;
    } else if ((i >= 10) AND (i < 36)) {
        ch = 'A' + i - 10;
    }
    return ch;
} // GetRegChar

void WriteReg(Ui::MainWindow *ui) {
/*
 struct RegisterType {
    double value;
    QString name;
 };
ARRAYOF RegisterType Storage[36];  // var Storage []RegisterType  in Go syntax

enum OutputStateEnum {outputfix, outputfloat, outputgen};
OutputStateEnum OutputState = outputfix;
 */
  stringstream stream;

  stream << "The following registers are not zero.";
  for (int i = 0; i < 36; i++) {
      if (Storage[i].value != 0.) {
          stream << "Reg [" << GetRegChar(i) << "]: ";
          stream << Storage[i].name.toStdString() << "= ";
          if (OutputState == outputfix) {
              stream.setf(ios::fixed);
              stream.width(15);
              stream.precision(SigFig);
              stream << Storage[i].value << endl;

          } else if (OutputState == outputfloat) {
              stream.setf(ios::scientific);
              stream.width(15);
              stream.precision(SigFig);
              stream << Storage[i].value << endl;

          } else {
              string str;
              str = to_string(Storage[i].value);
              stream << str << endl;

          }

      }
  }
  stream.flush();
  ui->listWidget_Registers->clear();
  QString qstr = QString::fromStdString(stream.str());
  ui->listWidget_Registers->addItem(qstr);
} // WriteReg()

void WriteHelp(QWidget *parent) {  // this param is intended so that 'this' can be used in the QMessageBox call.
    stringstream stream;
    calcPairType calcpairvar;
    vector<string> stringslice;
    vector<string>::iterator iterate;

    calcpairvar = GetResult("HELP");

    IF NOT calcpairvar.ss.empty() THEN
      for (iterate = calcpairvar.ss.begin(); iterate != calcpairvar.ss.end(); iterate++) {
         stream << *iterate  << endl;
      }

      calcpairvar.ss.clear();
      stream.flush();

      QString qs = QString::fromStdString(stream.str());
      QMessageBox::information(parent,"Help", qs);
    ENDIF;
}

void FUNCTION repaint(Ui::MainWindow *ui) {
  WriteStack(ui);
  WriteReg(ui);
} // repaint()

QString FUNCTION GetNameString(QWidget *parent) {
  QString prompt = "Input name, will make - or = into a space : ";
  bool ok;

  QString text = QInputDialog::getText(parent,"Enter Name for Storage Register", prompt,QLineEdit::Normal,"",&ok);
  if (NOT ok) {
      return "";
  }
  if ((text == "t") OR (text == "today")) {
      MDYType mdy = TIME2MDY();
      string datestring, dstr;
      datestring = MDY2STR(mdy.m,mdy.d,mdy.y, dstr);
      text = text.fromStdString(datestring);
      return text;
  }
  return text;  // compiler flagged this as possible end of function without a return.  I don't think this line can be reached, but it makes the compiler happy, so it's here.
}

string FUNCTION toupper(string s) {
    string upper = "";
    string::iterator iter;
    for (iter = s.begin(); iter != s.end(); iter++) {
        upper += CAP(*iter);
    }
    return upper;
}

void FUNCTION ProcessInput(QWidget *parent, Ui::MainWindow *ui, string cmdstr) {
    string LastCompiled = LastCompiledDate + " " + LastCompiledTime;
    calcPairType calcpair;
    vector<string> stringslice;
    vector<string>::iterator iter;

//    PushStacks();  looks like there were too many calls to PushStacks();  When I commented this out, undo started to work.

    // Write cmdstr to the history box, ie, listWidget_History.
    QString qs = QString::fromStdString(cmdstr);
    ui->listWidget_History->addItem(qs);

    if (cmdstr.compare("help") == 0) {   // help
        WriteHelp(parent);
    } else if (cmdstr.find("sto") EQ 0) {  // stoN
        int i = 0;
        char ch = '\0';
        if (cmdstr.length() > 3) {
            ch = cmdstr.at(3);  // the 4th character, right after the 'o' in 'sto'
            i = GetRegIdx(ch);
        }
        Storage[i].value = READX();
        if (i > 0) Storage[i].name = GetNameString(parent);

    } else if (cmdstr.find("rcl") EQ 0) {  // rclN
        int i = 0;
        if (cmdstr.length() > 3) {
            char ch = cmdstr.at(3); // the 4th char.
            i = GetRegIdx(ch);
        }
        double R = Storage[i].value;
        PUSHX(R);

    } else if (cmdstr.find("name") EQ 0) {
        int i = 0;
        if (cmdstr.length() > 4 ) {
            char ch = cmdstr.at(4); // the 5th char.
            i = GetRegIdx(ch);
        }
        Storage[i].name = GetNameString(parent);

    } else if ((cmdstr.compare("dump") EQ 0) OR (cmdstr.find("sho") EQ 0)) {
        // do nothing
    } else if (cmdstr.compare("about") EQ 0) {
        calcpair = GetResult("ABOUT");
        for (iter = calcpair.ss.begin(); iter != calcpair.ss.end(); iter++) {
            QString qstr = iter->c_str(); // recall that the -> operator is a type of dereference operator.
            ui->listWidget_Output->addItem(qstr);
        }

        string aboutmsg = "MainWindow Pgm last compiled " + LastCompiledDate + " " + LastCompiledTime;
        QString qaboutmsg = aboutmsg.c_str();  // only works when c-string.

        ui->listWidget_Output->addItem(qaboutmsg);

    } else if (cmdstr.compare("debug") EQ 0) {
            GETSTACK(Stk);
            stringstream stream, stream1, stream2, stream3;
            vector<double> stackmatrixrow;

            stream << "Raw Stk[] with X first is : ";

            FOR int i = X; i < StackSize; i++ DO
              string str = to_string(Stk[i]);
              stream << str  << "  ";
            ENDFOR;

            QString qoutput = QString::fromStdString(stream.str());
            ui->listWidget_Output->addItem(qoutput);

            stackmatrixrow = GetStackMatrixRow(X);
            stream1 << "First row of StackMatrix is :";

            FOR int i = X; i < StackSize; i++ DO
              string str = to_string(stackmatrixrow.at(i));
              stream1 << str  << "  ";
            ENDFOR;

            qoutput = "";
            qoutput = QString::fromStdString(stream1.str());
            ui->listWidget_Output->addItem(qoutput);

            stackmatrixrow = GetStackMatrixRow(Y);
            stream2 << "Second row of StackMatrix is :";

            FOR int i = X; i < StackSize; i++ DO
              string str = to_string(stackmatrixrow.at(i));
              stream2 << str  << "  ";
            ENDFOR;

            qoutput = "";
            qoutput = QString::fromStdString(stream2.str());
            ui->listWidget_Output->addItem(qoutput);

            stackmatrixrow = GetStackMatrixRow(Z);
            stream3 << "3rd row of StackMatrix is : ";

            FOR int i = X; i < StackSize; i++ DO
              string str = to_string(stackmatrixrow.at(i));
              stream3 << str << "  ";
            ENDFOR;
            stream3 << endl;

            qoutput = "";
            qoutput = QString::fromStdString(stream3.str());
            ui->listWidget_Output->addItem(qoutput);


    } else {
        // cmdstr = toupper(cmdstr); this works, but it's not needed.
        // QString qcmdstr = QString::fromStdString(cmdstr);
        // QMessageBox::information(parent,"cmdstr is", qcmdstr);
        calcpair = GetResult(cmdstr);

        if (NOT calcpair.ss.empty()) {
            for (iter = calcpair.ss.begin(); iter != calcpair.ss.end(); iter++) {
                QString qstr = iter->c_str(); // recall that the -> operator is a type of dereference operator.
                ui->listWidget_Output->addItem(qstr);
            }
        }

        QString qR1, qR2, qR3, qRfix, qRfloat, qRgen;
        string str = to_string(calcpair.R);
        str = CropNStr(str);

        if (calcpair.R > 10000) str = AddCommas(str);

        qR1 = QString("%1").arg(str.c_str());
        qR2 = QString::fromStdString(str);
        qR3 = QString("%1").arg(calcpair.R);
        qRgen = QString("%1").arg(calcpair.R,5,'g',SigFig);// params are: double,int fieldwidth=0, char format='g', int precision= -1 ,QChar fillchar = QLatin1Char(' ').
        qRfix = QString("%1").arg(calcpair.R,2,'f',SigFig);  // more general form of the conversion which can use 'e', 'f' and sigfig.
        qRfloat = QString("%1").arg(calcpair.R,9,'e',SigFig);  // not working.
        QString qoutputline;
        qoutputline = "qR1= " + qR1 + ", qR2= " + qR2 + ", qR3= " + qR3 + ", qRgen= " + qRgen + ", qRfix= " + qRfix + ", qRfloat= " + qRfloat;
//        ui->listWidget_Output->clear();  not yet.  I added menu option to clear the output area
//        QMessageBox::information(parent,"qoutputline", qoutputline);
//        ui->listWidget_Output->addItem(qoutputline);  these all work now.  I needed to use a new syntax.
    } // else from if input "help"

// I have to WriteStack, WriteReg, WriteHelp, output command line list, process some commands not run thru GetResult, like stoN, rclN, fix, float, gen, exit, quit, sigfig,
// have command not found error displayed
// if cmdstr is empty, display message to the output area to exit either type the commands exit, quit, or click the button.
// maybe I can use the menu structure to also change the output state and sigfig variables.

    repaint(ui);

    // clear the input lineedit before returning to it
    ui->lineEdit->clear();

}


void MainWindow::on_lineEdit_returnPressed() {
    QString inlineedit = ui->lineEdit->text();
    string inbuf = inlineedit.toStdString();
    inbuf = makesubst(inbuf);
    ProcessInput(this, ui, inbuf);
    //show();  not needed.
}

void MainWindow::on_pushButton_enter_clicked() {
    QString inlineedit = ui->lineEdit->text();
    string inbuf = inlineedit.toStdString();
    inbuf = makesubst(inbuf);
    ProcessInput(this, ui, inbuf);
    //show();  not needed.
}

void MainWindow::on_pushButton_exit_clicked() {
    GETSTACK(Stk);

// need to write the file
    // QMessageBox::information(this,"filename", CombinedFilenamestring);  It's working, so I don't need this.
    QFile file(CombinedFilenamestring);
    if (! file.open(QFile::WriteOnly)) {  // need to match up what's read and what's written.
        QMessageBox::warning(this,"open failed","file could not be opened in write mode.");
        return;
    }
    QDataStream stackout(&file);

    FOR int i = T1; i >= X; i-- DO  // need to do in reverse so it's read in correctly.  This is a stack, so LIFO.
      stackout << Stk[i];
    ENDFOR;

    FOR int i = 0; i < 36; i++ DO
      stackout << Storage[i].value;
      stackout << Storage[i].name;
    ENDFOR;

    file.flush();
    file.close();


    QApplication::quit();      // same as QCoreApplication::quit();

    // if the event loop is not running, these quit() commands will not work.  In that case, need to call exit(EXIT_FAILURE);
    exit(EXIT_FAILURE);
}

void MainWindow::on_pushButton_quit_clicked() {
//    GETSTACK(Stk);
// need to write the file for exit button, but I'll see how it goes for quit button not saving the state.
    QApplication::quit();      // same as QCoreApplication::quit();

    // if the event loop is not running, these quit() commands will not work.  In that case, need to call exit(EXIT_FAILURE);
    exit(EXIT_FAILURE);
}

void MainWindow::on_action2_triggered() {
    SigFig = 0;
    GetResult("sig0");
}

void MainWindow::on_action4_triggered() {
    SigFig = 2;
    GetResult("sig2");
}

void MainWindow::on_action4_2_triggered() {
    SigFig = 4;
    GetResult("fix4");
}

void MainWindow::on_action6_triggered() {
    SigFig = 6;
    GetResult("fix6");
}

void MainWindow::on_action8_triggered() {
    SigFig = 8;
    GetResult("sig8");
}

void MainWindow::on_action_1_triggered() {
    SigFig = -1;
    GetResult("sig");
}

void MainWindow::on_actionfixed_triggered() {
    OutputState = outputfix;
}

void MainWindow::on_actionfloat_triggered() {
    OutputState = outputfloat;
}

void MainWindow::on_actiongen_triggered() {
    OutputState = outputgen;
}

void MainWindow::on_actionClear_Output_triggered()
{
    ui->listWidget_Output->clear();
}
