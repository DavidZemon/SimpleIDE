#include "newproject.h"
#include "properties.h"

/*
 * Path is not saved in linux without installer - added workspace to properties dialog.
 */
NewProject::NewProject(QWidget *parent) : QDialog(parent)
{
    mypath = getCurrentPath();
    path = new QLineEdit(mypath,this);
    path->setToolTip(tr("Directory for new project."));

    QPushButton *btnBrowsePath = new QPushButton(this);
    btnBrowsePath->setText(tr("Browse"));

    name = new QLineEdit(this);
    name->setToolTip(tr("New project name and sub-directory."));
    connect(name,SIGNAL(textChanged(QString)),this,SLOT(nameChanged()));

    path->setText(mypath+name->text());
    QLabel *nameLabel = new QLabel(tr("Project Name"));
    QLabel *pathLabel = new QLabel(tr("Folder"));

    QDialogButtonBox *buttonBox = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel);
    connect(btnBrowsePath, SIGNAL(clicked()), this, SLOT(browsePath()));
    connect(buttonBox, SIGNAL(accepted()), this, SLOT(accept()));
    connect(buttonBox, SIGNAL(rejected()), this, SLOT(reject()));

    QVBoxLayout *blay = new QVBoxLayout(this);
    QGridLayout *layout = new QGridLayout();
    layout->addWidget(nameLabel,1,0,1,1);
    layout->addWidget(name,1,1,1,1);
    layout->addWidget(pathLabel,0,0,1,1);
    layout->addWidget(path,0,1,1,1);
    layout->addWidget(btnBrowsePath,0,2,1,1);

    blay->addLayout(layout);
    blay->addWidget(buttonBox);
    setLayout(blay);
    buttonBox->setFocus();

    int fontSize = path->fontInfo().pixelSize();
    setMinimumWidth(mypath.length()*fontSize+100);
    setWindowFlags(Qt::Tool);
}

NewProject::~NewProject()
{
    delete name;
    delete path;
}

void NewProject::nameChanged()
{
    path->setText(mypath+name->text());
}

QString NewProject::getCurrentPath()
{
    QSettings settings(publisherKey, ASideGuiKey, this);
    QVariant  lastfile = settings.value(workspaceKey);
    QString userpath("");
    if(lastfile.canConvert(QVariant::String)) {
        userpath = lastfile.toString();
        userpath = QDir::fromNativeSeparators(userpath);
        /* "/" gets converted to user OS version */
        userpath = userpath.mid(0,userpath.lastIndexOf("/")+1);
    }
    return userpath;
}

void NewProject::browsePath()
{
    QString pathName;
    QString fullname = mypath; //+name->text();
    QFileDialog fileDialog(this,tr("New Project Folder"),fullname,tr("Project Folder (*)"));
    QStringList filenames;
    fileDialog.setOptions(QFileDialog::ShowDirsOnly);
    fileDialog.setViewMode(QFileDialog::Detail);
    fileDialog.setFileMode(QFileDialog::Directory);
    if(mypath.at(mypath.length()-1) == '/')
        mypath = mypath.left(mypath.length()-1);
    fileDialog.selectFile(mypath);

    if(fileDialog.exec())
        filenames = fileDialog.selectedFiles();
    if(filenames.length() > 0)
        pathName = filenames.at(0);

    mypath = QDir::fromNativeSeparators(pathName);
    if(mypath.length() == 0)
        return;
    if(mypath.indexOf("/") > -1) {
        if(mypath.mid(mypath.length()-1) != "/")
            mypath += "/";
    }
    else if(mypath.indexOf("\\") > -1) {
        if(mypath.mid(mypath.length()-1) != "\\")
            mypath += "\\";
    }
    path->setText(mypath+name->text());
    QSettings settings(publisherKey, ASideGuiKey, this);
    settings.setValue(workspaceKey,mypath);
    int fontSize = path->fontInfo().pixelSize();
    setMinimumWidth(mypath.length()*fontSize+100);
    qDebug() << "New Project Folder " << mypath << name->text();
}

void NewProject::accept()
{
    if(name->text().length()<1) {
        QMessageBox::information(this,
                 tr("Need Project Name"),
                 tr("Please enter a project name."),QMessageBox::Ok);
        return;
    }
    int rc = QMessageBox::question(this,
                 tr("Confirm New Project"),
                 tr("Create new project file \"")+name->text()+".side\""+tr(" in\n")+path->text()+tr("?"),
                 QMessageBox::Yes, QMessageBox::No);
    if(rc == QMessageBox::Yes) {
        path->setText(mypath+name->text());
        done(QDialog::Accepted);
    }
}

void NewProject::reject()
{
    done(QDialog::Rejected);
}

void NewProject::showDialog()
{
    int fontSize = path->fontInfo().pixelSize();
    setMinimumWidth(mypath.length()*fontSize+100);

    mypath = getCurrentPath();
    path->setText(mypath+name->text());
    this->setWindowTitle(QString(ASideGuiKey)+tr(" New Project"));
    this->exec();
}

QString NewProject::getName()
{
    return name->text();
}

QString NewProject::getPath()
{
    return path->text();
}
