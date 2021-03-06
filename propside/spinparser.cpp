/*
 * This file is part of the Parallax Propeller SimpleIDE development environment.
 *
 * Copyright (C) 2014 Parallax Incorporated
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include <QWidget>
#include "spinparser.h"

#define KEY_ELEMENT_SEP ":"

SpinParser::SpinParser()
{
    setKind(&SpinKinds[SpinParser::K_NONE],     false,'n', "none", "none"); // place-holder only
    setKind(&SpinKinds[SpinParser::K_CONST],    true, 'c', "constant", "constants");
    setKind(&SpinKinds[SpinParser::K_PUB],      true, 'f', "public", "methods");
    setKind(&SpinKinds[SpinParser::K_PRI],      true, 'p', "private", "functions");
    setKind(&SpinKinds[SpinParser::K_OBJECT],   true, 'o', "obj", "objects");
    setKind(&SpinKinds[SpinParser::K_VAR],      true, 'v', "var", "variables");
    setKind(&SpinKinds[SpinParser::K_DAT],      true, 'x', "dat", "dat");

    KeyWord keyCon = {"con", K_CONST, 0};
    KeyWord keyObj = {"obj", K_OBJECT, 0};
    KeyWord keyPub = {"pub", K_PUB, 0};
    KeyWord keyPri = {"pri", K_PRI, 0};
    KeyWord keyVar = {"var", K_VAR, 0};
    KeyWord keyDat = {"dat", K_DAT, 0};
    KeyWord keyNull= {NULL,  K_NONE, 0};

    spin_keywords.append(keyCon);
    spin_keywords.append(keyObj); // keep as item 1.
    spin_keywords.append(keyPub);
    spin_keywords.append(keyPri);
    spin_keywords.append(keyVar);
    spin_keywords.append(keyDat);
    spin_keywords.append(keyNull);
}

SpinParser::~SpinParser()
{
    clearDB();
}

void SpinParser::clearDB()
{
    db.clear();
    spinFiles.clear();
}

void SpinParser::setKind(kindOption *kind, bool en, const char letter, const char *name, const char *desc)
{
    kind->enabled = en;
    kind->letter = letter;
    kind->name = name;
    kind->description = desc;
}

/*
 * get a tree list.
 * all object instances will be listed with sub-objects
 * the list will be indented according to relative position
 */
QStringList SpinParser::spinFileTree(QString file, QString libpath)
{
    QString key;
    QString value;
    QString subnode;
    QString subfile;

    libraryPath = libpath;
    clearDB();

    this->findSpinTags(file, "root");

    spinFiles.append(file.mid(file.lastIndexOf("/")+1));
    QStringList keys = db.keys();

#if defined(SPIN_AUTOCOMPLETE)
    makeTags(file);
#endif

    for(int n = 0; n < keys.length(); n++) {
        key = keys[n];
        value = db[key];
        QStringList levels = key.split("/");
        int lcount = levels.count()-1;
        QStringList keyel = levels[lcount].split(KEY_ELEMENT_SEP);
        if(keyel.count() < 2)
            continue;
        if(QString(keyel[0]).compare(keyel[1]) == 0) {
            objectInfo(value, subnode, subfile);
            //for(int n = 0; n < lcount; n++) subfile = " " + subfile;
            spinFiles.append(subfile);
        }
    }

    return spinFiles;
}

void SpinParser::makeTags(QString file)
{
    QStringList keys = db.keys();

    QString tagheader = \
"!_TAG_FILE_FORMAT	1	/original ctags format/\n"
"!_TAG_FILE_SORTED	1	/0=unsorted, 1=sorted, 2=foldcase/\n"
"!_TAG_PROGRAM_AUTHOR	Darren Hiebert	/dhiebert@users.sourceforge.net/\n"
"!_TAG_PROGRAM_NAME	Exuberant Ctags	//\n"
"!_TAG_PROGRAM_URL	http://ctags.sourceforge.net	/official site/\n"
"!_TAG_PROGRAM_VERSION	5.8	//\n";

    QString path = file.mid(0,file.lastIndexOf("/")+1);
    QFile tags(path+"tags");
    if(tags.open(QFile::WriteOnly | QFile::Text)) {
        tags.write(tagheader.toLatin1());
        foreach(QString key, keys) {
            QStringList tl = db[key].split("\t");
            QString ts = tl[0]+"\t"+tl[1]+"\t/^"+tl[2]+"$/";
            tags.write(ts.toLatin1()+"\n");
        }
        tags.close();
    }

}

QString SpinParser::tagItem(QStringList tabs, int field)
{
    QString s;
    if(tabs.length() > 2)
        s = tabs.at(3)+"\t"+tabs.at(field);
    return s;
}

/*
 * all symbols are accessible by key.
 * a key is a name list such as "/root/obj/subobj/subsubobj"
 * if the key is empty, return the root or top file list
 */
QStringList SpinParser::spinSymbols(QString file, QString objname)
{
    QStringList list;
    QStringList keys = db.keys();
    QString key;
    QString value;
    for(int n = 0; n < keys.length(); n++) {
        key = keys[n];
        value = db[key];
        if(objname.length() > 0) {
            if(key.contains(objname+":",Qt::CaseInsensitive)) {
                QStringList tabs = value.split("\t");
                if(tabs.count() > 2)
                    list.append(tagItem(tabs,2));
            }
        }
        else if(value.contains(file,Qt::CaseInsensitive)) {
            QStringList tabs = value.split("\t");
            if(tabs.count() > 2)
                list.append(tagItem(tabs,2));
        }
    }
    return list;
}

/*
 * these are convenience wrappers for typefilter
 */
QStringList SpinParser::spinConstants(QString file, QString objname)
{
    QStringList list;
    QStringList keys = db.keys();
    QString key;
    QString value;
    for(int n = 0; n < keys.length(); n++) {
        key = keys[n];
        value = db[key];
        if(objname.length() > 0) {
            if(key.contains(objname+":",Qt::CaseInsensitive)) {
                QStringList tabs = value.split("\t");
                if(tabs.count() > 2) {
                    if(QString(tabs.at(3)).contains("c",Qt::CaseInsensitive))
                        list.append(tagItem(tabs,2));
                    else if(QString(tabs.at(3)).contains("e",Qt::CaseInsensitive)) {
                        list.append(tagItem(tabs,0));
                    }
                }
            }
        }
        else if(value.contains(file,Qt::CaseInsensitive)) {
            QStringList tabs = value.split("\t");
            if(tabs.count() > 2) {
                if(QString(tabs.at(3)).contains("c",Qt::CaseInsensitive))
                    list.append(tagItem(tabs,2));
                else if(QString(tabs.at(3)).contains("e",Qt::CaseInsensitive)) {
                    list.append(tagItem(tabs,0));
                }
            }
        }
    }
    return list;
}

QStringList SpinParser::spinMethods(QString file, QString objname)
{
    QStringList list;
    QStringList keys = db.keys();
    QString key;
    QString value;
    for(int n = 0; n < keys.length(); n++) {
        key = keys[n];
        value = db[key];
        if(objname.length() > 0) {
            if(key.contains(objname+":",Qt::CaseInsensitive)) {
                QStringList tabs = value.split("\t");
                if(tabs.count() > 2) {
                    if(QString(tabs.at(3)).contains("p",Qt::CaseInsensitive))
                        list.append(tagItem(tabs,2));
                    if(QString(tabs.at(3)).contains("f",Qt::CaseInsensitive))
                        list.append(tagItem(tabs,2));
                }
            }
        }
        else if(value.contains(file,Qt::CaseInsensitive)) {
            QStringList tabs = value.split("\t");
            if(tabs.count() > 2) {
                if(QString(tabs.at(3)).contains("p",Qt::CaseInsensitive))
                    list.append(tagItem(tabs,2));
                if(QString(tabs.at(3)).contains("f",Qt::CaseInsensitive))
                    list.append(tagItem(tabs,2));
                if(QString(tabs.at(3)).contains("o",Qt::CaseInsensitive))
                    list.append(tagItem(tabs,2));
            }
        }
    }
    return list;
}

QStringList SpinParser::spinDat(QString file, QString objname)
{
    QStringList list;
    QStringList keys = db.keys();
    QString key;
    QString value;
    for(int n = 0; n < keys.length(); n++) {
        key = keys[n];
        value = db[key];
        if(key.contains(objname+":",Qt::CaseInsensitive)) {
            QStringList tabs = value.split("\t");
            if(tabs.count() > 2) {
                if(QString(tabs.at(3)).contains("d",Qt::CaseInsensitive))
                    list.append(tagItem(tabs,2));
            }
        }
    }
    return list;
}

QStringList SpinParser::spinVars(QString file, QString objname)
{
    QStringList list;
    QStringList keys = db.keys();
    QString key;
    QString value;
    for(int n = 0; n < keys.length(); n++) {
        key = keys[n];
        value = db[key];
        if(key.contains(objname+":",Qt::CaseInsensitive)) {
            QStringList tabs = value.split("\t");
            if(tabs.count() > 2) {
                if(QString(tabs.at(3)).contains("v",Qt::CaseInsensitive))
                    list.append(tagItem(tabs,2));
            }
        }
    }
    return list;
}

QStringList SpinParser::spinObjects(QString file, QString objname)
{
    QStringList list;
    QStringList keys = db.keys();
    QString key;
    QString value;
    for(int n = 0; n < keys.length(); n++) {
        key = keys[n];
        value = db[key];
        if(key.contains(objname+":",Qt::CaseInsensitive)) {
            QStringList tabs = value.split("\t");
            if(tabs.count() > 2) {
                if(QString(tabs.at(3)).contains("o",Qt::CaseInsensitive))
                    list.append(tagItem(tabs,2));
            }
        }
    }
    return list;
}

/*
 *   FUNCTION DEFINITIONS
 */

/* Match the name of a tag (function, variable, type, ...) starting at pos. */
char const *SpinParser::extract_name (char const *pos, QString &name)
{
    while (isspace (*pos))
        pos++;
    name.clear();
    for (; *pos && !isspace (*pos) && *pos != '(' && *pos != ','; pos++)
        name.append(*pos);
    return pos;
}

int SpinParser::tokentype(QString tmp)
{
    int rc = K_NONE;
    int i;
    /* find out token type for parser states */
    if(tmp.indexOf(' ') > 0) {
        tmp = tmp.mid(0,tmp.indexOf(' '));
    }
    for (i = 0; spin_keywords[i].token != NULL; i++)
    {
        if (tmp.compare(spin_keywords[i].token,Qt::CaseInsensitive) == 0) {
            rc = spin_keywords[i].kind;
            break;
        }
    }
    return rc;
}

/* Match a keyword starting at p (case insensitive). */
int SpinParser::match_keyword (const char *p, KeyWord const *kw, QString &tag)
{
    QString name;
    int i;
    int j;
    int rc = K_NONE;
    char *tmp = (char*)p;

    /* printf ("match_keyword\n"); */

    for (i = 0; i < kw->token.length(); i++)
    {
        if (tolower (p[i]) != kw->token[i])
            return rc;
    }

    p += i;
    if(*p && !isspace(*p))
        return K_NONE;

    /* if p stands alone on the line, don't add it */
    if(strlen(p) == 0)
         return tokentype(tmp);

    for (j = 0; j < 1 + kw->skip; j++)
    {
        p = extract_name (p, name);
    }
    /* printf ("match_keyword name %d %s\n", name->length, name->buffer); */

    if(name.length() > 0) {
        rc = tokentype(kw->token);
        tag = name+"\t"+currentFile+"\t"+tmp+"\t"+SpinKinds[rc].letter;
    }
    return rc;
}

void SpinParser::match_constant (QString p)
{
    QString tag("");
    int len;
    bool ok;
    p = p.trimmed();
    // ENUM is a constant like #n, NAME, NAME2
    if((p.indexOf("#")) == 0) {
        // add to database
        QString s = p.mid(1).trimmed();
        QStringList list = s.split(",");
        for(int n = 0; n < list.count(); n++) {
            s = QString(list[n]).trimmed();
            s.toInt(&ok);  // don't add numbers to the list
            if(ok == true) continue;
            tag = s+"\t"+currentFile+"\t"+p+"\t"+"e";
            db.insert(objectNode+KEY_ELEMENT_SEP+s,tag);
            // qDebug() << objectNode << " ## " << tag;
        }
    }
    else if((len = p.indexOf("=")) > 0) {
        if(p.contains(QRegExp("\\b?=\\b")))
            len = 0;
        if(len > 0) {
            // add to database
            QString s = p.mid(0,len).trimmed();
            if(s.indexOf("con",0,Qt::CaseInsensitive) == 0)
                s = s.mid(4);
            s = s.trimmed();
            tag = s+"\t"+currentFile+"\t"+p+"\t"+SpinKinds[K_CONST].letter;
            db.insert(objectNode+KEY_ELEMENT_SEP+s,tag);
            // qDebug() << objectNode << " ## " << tag;
        }
    }
}

void SpinParser::match_dat (QString p)
{
    QString tag = "";
    QString s = p.trimmed();
    if(s.indexOf("dat",0,Qt::CaseInsensitive) == 0)
        s = s.mid(4);
    s = s.trimmed();
    QRegExp regex("\\b(byte|long|word)\\b");
    regex.setCaseSensitivity(Qt::CaseInsensitive);
    if(s.contains(regex)) {
        s = s.mid(0,s.indexOf(regex));
        s = s.trimmed();
        QStringList list = s.split(",",QString::SkipEmptyParts);
        if(list.length() > 0) {
            foreach(s,list) {
                if(s.contains("["))
                    s = s.mid(0,s.indexOf("["));
                s = s.trimmed();
                tag = s+"\t"+currentFile+"\t"+p+"\t"+SpinKinds[K_DAT].letter;
                db.insert(objectNode+KEY_ELEMENT_SEP+s,tag);
                // qDebug() << objectNode << " DAT " << tag;
            }
        }
    }
}

void SpinParser::match_object (QString p)
{
    QString tag = "";
    QString subfile = "";
    QString subnode = "";
    int len = p.indexOf(":");
    if(p.indexOf(":=") > 0)
        len = 0;
    if(len > 0) {
        // add to database
        QString s = p.mid(0,len);
        if(s.contains("OBJ",Qt::CaseInsensitive))
            s = s.mid(s.indexOf("OBJ",0,Qt::CaseInsensitive)+3);
        if(s.contains("[",Qt::CaseInsensitive))
            s = s.mid(0,s.indexOf("[",0,Qt::CaseInsensitive));
        s = s.trimmed();
        tag = s+"\t"+currentFile+"\t"+p+"\t"+SpinKinds[K_OBJECT].letter;
        objectInfo(tag, subnode, subfile);
        QString file = checkFile(subfile);
        if(QFile::exists(file) == false)
            return;
        db.insert(objectNode+"/"+subnode+KEY_ELEMENT_SEP+s,tag);
        // qDebug() << objectNode+"/"+subnode << " :: " << tag;
        findSpinTags(subfile,objectNode+"/"+subnode);
    }
}

void SpinParser::match_pri (QString p)
{
    QString tag("");
    int len = p.indexOf("pri",0,Qt::CaseInsensitive);
    if(len == 0) {
        // add to database
        QString s = p.mid(4);
        if(s.indexOf("|") >= 0)
            s = s.mid(0,s.indexOf("|"));
        if(s.indexOf(":") >= 0)
            s = s.mid(0,s.indexOf(":"));
        if(s.indexOf("(") >= 0)
            s = s.mid(0,s.indexOf("("));
        s = s.trimmed();
        tag = s+"\t"+currentFile+"\t"+p+"\t"+SpinKinds[K_PRI].letter;
        db.insert(objectNode+KEY_ELEMENT_SEP+s,tag);
        // qDebug() << objectNode << " pri " << tag;
    }
}

void SpinParser::match_pub (QString p)
{
    QString tag("");
    int len = p.indexOf("pub",0,Qt::CaseInsensitive);
    if(len == 0) {
        // add to database
        QString s = p.mid(4);
        if(s.indexOf("|") >= 0)
            s = s.mid(0,s.indexOf("|"));
        if(s.indexOf(":") >= 0)
            s = s.mid(0,s.indexOf(":"));
        if(s.indexOf("(") >= 0)
            s = s.mid(0,s.indexOf("("));
        s = s.trimmed();
        tag = s+"\t"+currentFile+"\t"+p+"\t"+SpinKinds[K_PUB].letter;
        db.insert(objectNode+KEY_ELEMENT_SEP+s,tag);
        // qDebug() << objectNode << " pub " << tag;
    }
}

void SpinParser::match_var (QString p)
{
    QString tag = "";
    QString s = p.trimmed();
    if(s.indexOf("var",0,Qt::CaseInsensitive) == 0)
        s = s.mid(4);
    s = s.trimmed();
    QRegExp regex("\\b(byte|long|word)\\b");
    regex.setCaseSensitivity(Qt::CaseInsensitive);
    if(s.contains(regex)) {
        QString s = p.trimmed();
        if(s.indexOf("var",0,Qt::CaseInsensitive) == 0)
            s = s.mid(4);
        s = s.trimmed();
        if(s.indexOf(regex) == 0) {
            s = s.mid(s.indexOf(regex)+4);
            QStringList list = s.split(",",QString::SkipEmptyParts);
            if(list.length() > 0) {
                foreach(s,list) {
                    if(s.contains("["))
                        s = s.mid(0,s.indexOf("["));
                    s = s.trimmed();
                    tag = s+"\t"+currentFile+"\t"+p+"\t"+SpinKinds[K_VAR].letter;
                    db.insert(objectNode+KEY_ELEMENT_SEP+s,tag);
                    // qDebug() << objectNode << " VAR " << tag;
                }
            }
        }
    }
}

int SpinParser::objectInfo(QString tag, QString &name, QString &file)
{
    QStringList list = tag.split("\t",QString::KeepEmptyParts);
    if(list.length() == 0)
        return 0;
    name = QString(list[0]).trimmed();
    file = QString(list[2]).trimmed();
    file = file.mid(file.indexOf("\"")+1);
    file = file.mid(0,file.indexOf("\""));
    file = file.trimmed();

    if(file.contains(".spin",Qt::CaseInsensitive) == false)
        file += ".spin";

    return list.length();
}


QString SpinParser::checkFile(QString fileName)
{
    QString retfile = fileName;

    if(QFile::exists(fileName) == true) {
        return retfile;
    }
    else if(QFile::exists(libraryPath+fileName)) {
        return libraryPath+fileName;
    }
    else {
        QDir dir;
        QStringList list;
        QString fs = this->currentFile;
        QString shortfile = fileName.mid(fileName.lastIndexOf("/")+1);
        QString path = fs.mid(0,fs.lastIndexOf("/")+1);
        dir.setPath(path);
        list = dir.entryList();
        foreach(QString s, list) {
            if(s.compare(shortfile,Qt::CaseInsensitive) == 0) {
                return path+s;
            }
        }
        dir.setPath(libraryPath);
        list = dir.entryList();
        foreach(QString s, list) {
            if(s.contains(shortfile,Qt::CaseInsensitive)) {
                return libraryPath+"/"+s;
            }
        }
    }
    return retfile;
}

void SpinParser::findSpinTags (QString fileName, QString objnode)
{
    fileName = checkFile(fileName);
    if(QFile::exists(fileName) == false)
        return;

    QString line;
    QString tag;
    QString filestr;

    SpinKind state = K_CONST; // spin starts with CONST
    bool blockComment = false;

    QFile file(fileName);
    QTextStream in(&file);
    in.setAutoDetectUnicode(true);
    if(file.open(QFile::ReadOnly) != true)
        return;
    QStringList list;
    QStringList lst2;

    filestr = in.readAll();
    file.close();

    list = filestr.split("\n",QString::SkipEmptyParts,Qt::CaseInsensitive);
    lst2 = filestr.split("\r",QString::SkipEmptyParts,Qt::CaseInsensitive);

    /* amazing that we have to do stuff like this with spin files */
    if(lst2.count() > list.count())
        list = filestr.split("\r",QString::SkipEmptyParts,Qt::CaseInsensitive);

    for(int n = 0; n < list.length(); n++)
    {
        // give app a chance to do work? parsing can take a while.
        // QApplication::processEvents();

        // do here in case OBJ detect causes context change
        objectNode  = objnode;
        currentFile = fileName;

        line = QString(list[n]).trimmed();

        if(line.contains("{") || line.contains("}")) {
            QString s = line;
            line = "";
            for(int j = 0; j < s.length(); j++) {
                QChar c = s[j];
                if(c == '{') {
                    blockComment = true;
                }
                else if(c == '}') {
                    blockComment = false;
                    j++;
                }
                if(blockComment == false)
                    line += s[j];
            }
        }
        if(blockComment)
            continue;

        if(line.contains("\'")) {
            line = line.mid(0,line.indexOf("\'"));
        }

        /* Empty line? */
        if (line.length() < 1)
            continue;

#if !defined(SPIN_AUTOCOMPLETE)
        /* In Spin, keywords always are at the start of the line. */
        SpinKind type = K_NONE;
        KeyWord kw = spin_keywords[1]; // K_OBJECT is not 1, but OBJ keyword is.
        line = line.trimmed();
        QRegExp tokens("\\b(con|obj|pri|pub|dat|var)\\b");
        tokens.setCaseSensitivity(Qt::CaseInsensitive);
        if(line.indexOf(tokens) == 0) {
            // if line has obj at position 0, get the type and store it
            // else set to some other type so we stop looking for objects
            if(line.indexOf("obj",0,Qt::CaseInsensitive) == 0)
                type = (SpinKind) match_keyword (line.toLatin1(), &kw, tag);
            else
                type = this->K_CONST;

            // keep state until it changes from K_NONE
            if(type != K_NONE) {
                state = type;
            }
        }

        //printf ("state %d\n", state);
        switch(state) {
            case K_OBJECT:
                match_object(line);
            break;
            default:
                tag = "";
            break;
        }
#else
        /* In Spin, keywords always are at the start of the line. */
        SpinKind type = K_NONE;
        foreach(KeyWord kw, spin_keywords) {
            if(kw.kind == K_NONE)
                continue;
            type = K_NONE;
            line = line.trimmed();
            if(line.indexOf(kw.token,0,Qt::CaseInsensitive) == 0) {
                // if line has a token, get the type and store it
                type = (SpinKind) match_keyword (line.toLatin1(), &kw, tag);

                // keep state until it changes from K_NONE
                if(type != K_NONE) {
                    state = type;
                    break;
                }
            }
        }

        //printf ("state %d\n", state);
        switch(state) {
            case K_CONST:
                match_constant(line);
            break;
            case K_DAT:
                match_dat(line);
            break;
            case K_OBJECT:
                match_object(line);
            break;
            case K_PRI:
                match_pri(line);
            break;
            case K_PUB:
                match_pub(line);
            break;
            case K_VAR:
                match_var(line);
            break;
            default:
                tag = "";
            break;
        }
#endif
    }
}
