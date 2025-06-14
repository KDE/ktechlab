/***************************************************************************
 *   Copyright (C) 2003-2005 by David Saxton                               *
 *   david@bluehaze.org                                                    *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 ***************************************************************************/

#include "projectmanager.h"
#include "docmanager.h"
#include "document.h"
#include "ktechlab.h"
#include "language.h"
#include "languagemanager.h"
#include "microselectwidget.h"
#include "programmerdlg.h"
#include "projectdlgs.h"

#include <KIO/FileCopyJob>
#include <KJobWidgets>
#include <KLocalizedString>
#include <KMessageBox>
#include <KRecentFilesAction>
#include <KXMLGUIFactory>

#include <QDomDocument>
#include <QDomElement>
#include <QFileDialog>
// #include <q3popupmenu.h>
#include <QDir>
#include <QMenu>
#include <QMimeDatabase>
#include <QMimeType>
#include <QScopedPointer>
#include <QStandardPaths>
#include <QTemporaryFile>

#include <cassert>

#include <ktlconfig.h>
#include <ktechlab_debug.h>

using Qt::Literals::operator""_L1;

static QString relativeUrl(const QUrl &baseDirUrl, const QUrl &url)
{
    if (baseDirUrl.scheme() == url.scheme() && baseDirUrl.host() == url.host() && baseDirUrl.port() == url.port() && baseDirUrl.userInfo() == url.userInfo()) {
        return QDir(baseDirUrl.path()).relativeFilePath(url.path());
    }

    return url.toDisplayString(QUrl::PreferLocalFile);
}

static QString resolvedLocalFile(const QString &baseDir, const QString &path)
{
    Q_ASSERT(baseDir.endsWith(QLatin1Char('/')));
    if (QDir::isAbsolutePath(path))
        return path;

    return QDir::cleanPath(baseDir + path);
}

// BEGIN class LinkerOptions
LinkerOptions::LinkerOptions()
{
    m_hexFormat = HexFormat::inhx32;
    m_bOutputMapFile = false;
}

QDomElement LinkerOptions::toDomElement(QDomDocument &doc, const QUrl &baseDirUrl) const
{
    QDomElement node = doc.createElement("linker"_L1);

    node.setAttribute("hex-format"_L1, hexFormatToString(hexFormat()));
    node.setAttribute("output-map-file"_L1, outputMapFile());
    node.setAttribute("library-dir"_L1, libraryDir());
    node.setAttribute("linker-script"_L1, linkerScript());
    node.setAttribute("other"_L1, linkerOther());

    // internal are always local files, like the project base dir
    // so can always get relative path from a QDir
    const QDir baseDir(baseDirUrl.toLocalFile());
    QStringList::const_iterator end = m_linkedInternal.end();
    for (QStringList::const_iterator it = m_linkedInternal.begin(); it != end; ++it) {
        QDomElement child = doc.createElement("linked-internal"_L1);
        node.appendChild(child);
        child.setAttribute("url"_L1, baseDir.relativeFilePath(*it));
    }

    end = m_linkedExternal.end();
    for (QStringList::const_iterator it = m_linkedExternal.begin(); it != end; ++it) {
        QDomElement child = doc.createElement("linked-external"_L1);
        node.appendChild(child);
        child.setAttribute("url"_L1, *it);
    }

    return node;
}

void LinkerOptions::domElementToLinkerOptions(const QDomElement &element, const QUrl &baseDirUrl)
{
    setHexFormat(stringToHexFormat(element.attribute("hex-format"_L1, QString())));
    setOutputMapFile(element.attribute("output-map-file"_L1, "0"_L1).toInt());
    setLibraryDir(element.attribute("library-dir"_L1, QString()));
    setLinkerScript(element.attribute("linker-script"_L1, QString()));
    setLinkerOther(element.attribute("other"_L1, QString()));

    m_linkedInternal.clear();
    m_linkedExternal.clear();

    QString baseDir = baseDirUrl.toLocalFile();
    if (!baseDir.endsWith(QLatin1Char('/'))) {
        baseDir.append(QLatin1Char('/'));
    }
    QDomNode node = element.firstChild();
    while (!node.isNull()) {
        QDomElement childElement = node.toElement();
        if (!childElement.isNull()) {
            const QString tagName = childElement.tagName();

            if (tagName == "linked-internal"_L1)
                m_linkedInternal << ::resolvedLocalFile(baseDir, childElement.attribute("url"_L1, QString()));
            else if (tagName == "linked-external"_L1)
                m_linkedExternal << childElement.attribute("url"_L1, QString());

            else
                qCCritical(KTL_LOG) << "Unrecognised element tag name: " << tagName;
        }

        node = node.nextSibling();
    }
}

QString LinkerOptions::hexFormatToString(HexFormat::type hexFormat)
{
    switch (hexFormat) {
    case HexFormat::inhx32:
        return "inhx32"_L1;

    case HexFormat::inhx8m:
        return "inhx8m"_L1;

    case HexFormat::inhx8s:
        return "inhx8s"_L1;

    case HexFormat::inhx16:
        return "inhx16"_L1;
    }

    // Default hex format is inhx32
    return "inhx32"_L1;
}

LinkerOptions::HexFormat::type LinkerOptions::stringToHexFormat(const QString &hexFormat)
{
    if (hexFormat == "inhx8m"_L1)
        return HexFormat::inhx8m;

    if (hexFormat == "inhx8s"_L1)
        return HexFormat::inhx8s;

    if (hexFormat == "inhx16"_L1)
        return HexFormat::inhx16;

    return HexFormat::inhx32;
}
// END class LinkerOptions

// BEGIN class ProcessingOptions
ProcessingOptions::ProcessingOptions()
{
    m_bUseParentMicroID = false;
    m_microID = "P16F84"_L1;
}

ProcessingOptions::~ProcessingOptions()
{
}

QDomElement ProcessingOptions::toDomElement(QDomDocument &doc, const QUrl &baseDirUrl) const
{
    QDomElement node = doc.createElement("processing"_L1);

    node.setAttribute("output"_L1, ::relativeUrl(baseDirUrl, outputURL()));
    node.setAttribute("micro"_L1, m_microID);

    return node;
}

void ProcessingOptions::domElementToProcessingOptions(const QDomElement &element, const QUrl &baseDirUrl)
{
    setOutputURL(baseDirUrl.resolved(QUrl(element.attribute("output"_L1, QString()))));
    setMicroID(element.attribute("micro"_L1, QString()));
}
// END class ProcessingOptions

// BEGIN class ProjectItem
ProjectItem::ProjectItem(ProjectItem *parent, Type type, ProjectManager *projectManager)
    : QObject()
{
    m_pParent = parent;
    m_pILVItem = nullptr;
    m_pProjectManager = projectManager;
    m_type = type;
}

ProjectItem::~ProjectItem()
{
    m_children.removeAll(static_cast<ProjectItem *>(nullptr));
    ProjectItemList::iterator end = m_children.end();
    for (ProjectItemList::iterator it = m_children.begin(); it != end; ++it)
        (*it)->deleteLater();
    m_children.clear();

    delete m_pILVItem;
}

void ProjectItem::setILVItem(ILVItem *ilvItem)
{
    m_pILVItem = ilvItem;
    ilvItem->setExpanded(true);
    ilvItem->setText(0, name());
    ilvItem->setProjectItem(this);
    updateILVItemPixmap();
}

void ProjectItem::updateILVItemPixmap()
{
    if (!m_pILVItem)
        return;

    switch (type()) {
    case ProjectType: {
        // ?! - We shouldn't have an ilvitem for this.
        break;
    }

    case ProgramType: {
        QPixmap pm;
        pm.load(QStandardPaths::locate(QStandardPaths::AppDataLocation, "icons/project_program.png"_L1));
        m_pILVItem->setIcon(0, QIcon(pm));
        break;
    }

    case LibraryType: {
        QPixmap pm;
        pm.load(QStandardPaths::locate(QStandardPaths::AppDataLocation, "icons/project_library.png"_L1));
        m_pILVItem->setIcon(0, QIcon(pm));
        break;
    }

    case FileType: {
        QMimeType m = QMimeDatabase().mimeTypeForFile(url().path());
        // m_pILVItem->setPixmap( 0, m->pixmap( KIconLoader::Small ) );
        m_pILVItem->setIcon(0, QIcon::fromTheme(m.iconName()));
        break;
    }
    }
}

void ProjectItem::addChild(ProjectItem *child)
{
    if (!child || m_children.contains(child))
        return;

    m_children << child;

    child->setILVItem(m_pILVItem ? new ILVItem(m_pILVItem, child->name()) : new ILVItem(m_pProjectManager, name()));

    updateControlChildMicroIDs();
}

void ProjectItem::updateControlChildMicroIDs()
{
    bool control = false;
    switch (type()) {
    case ProjectItem::ProjectType:
    case ProjectItem::LibraryType:
    case ProjectItem::ProgramType:
        control = !microID().isEmpty();
        break;

    case ProjectItem::FileType:
        control = true;
        break;
    }

    m_children.removeAll(static_cast<ProjectItem *>(nullptr));
    ProjectItemList::iterator end = m_children.end();
    for (ProjectItemList::iterator it = m_children.begin(); it != end; ++it)
        (*it)->setUseParentMicroID(control);
}

void ProjectItem::setObjectName(const QString &name)
{
    m_name = name;
    if (m_pILVItem)
        m_pILVItem->setText(0, name);
}

void ProjectItem::setURL(const QUrl &url)
{
    m_url = url;

    if (m_name.isEmpty())
        setObjectName(url.fileName());

    if (type() != FileType) {
        // The output url *is* our url
        setOutputURL(url);
    } else if (outputURL().isEmpty()) {
        // Try and guess what the output url should be...
        QString newExtension;

        switch (outputType()) {
        case ProgramOutput:
            newExtension = ".hex"_L1;
            break;

        case ObjectOutput:
            newExtension = ".o"_L1;
            break;

        case LibraryOutput:
            newExtension = ".o"_L1;
            break;

        case UnknownOutput:
            break;
        }

        if (!newExtension.isEmpty()) {
            QString fileName = url.path();
            fileName.chop(fileName.length() - fileName.lastIndexOf(QLatin1Char('.')));
            fileName.append(newExtension);
            QUrl newUrl(url);
            newUrl.setPath(fileName);
            setOutputURL(newUrl);
        }
    }

    updateILVItemPixmap();
}

QString ProjectItem::microID() const
{
    if (!m_bUseParentMicroID)
        return m_microID;

    return m_pParent ? m_pParent->microID() : QString();
}

void ProjectItem::setMicroID(const QString &id)
{
    ProcessingOptions::setMicroID(id);
    updateControlChildMicroIDs();
}

ProjectItem::OutputType ProjectItem::outputType() const
{
    if (!m_pParent)
        return UnknownOutput;

    switch (m_pParent->type()) {
    case ProjectItem::ProjectType: {
        // We're a top level build target, so look at our own type
        switch (type()) {
        case ProjectItem::ProjectType:
            qCWarning(KTL_LOG) << "Parent item and this item are both project items";
            return UnknownOutput;

        case ProjectItem::FileType:
        case ProjectItem::ProgramType:
            return ProgramOutput;

        case ProjectItem::LibraryType:
            return LibraryOutput;
        }
        return UnknownOutput;
    }

    case ProjectItem::FileType: {
        qCWarning(KTL_LOG) << "Don't know how to handle parent item being a file";
        return UnknownOutput;
    }

    case ProjectItem::ProgramType:
    case ProjectItem::LibraryType:
        return ObjectOutput;
    }

    return UnknownOutput;
}

bool ProjectItem::build(ProcessOptionsList *pol)
{
    if (!pol)
        return false;

    // Check to see that we aren't already in the ProcessOptionstList;
    ProcessOptionsList::iterator polEnd = pol->end();
    for (ProcessOptionsList::iterator it = pol->begin(); it != polEnd; ++it) {
        if ((*it).targetFile() == outputURL().toLocalFile())
            return true;
    }

    ProjectInfo *projectInfo = ProjectManager::self()->currentProject();
    assert(projectInfo);

    if (outputURL().isEmpty()) {
        KMessageBox::error(nullptr, i18n("Do not know how to build \"%1\" (output URL is empty).", name()));
        return false;
    }

    // Build all internal libraries that we depend on
    QStringList::iterator send = m_linkedInternal.end();
    for (QStringList::iterator it = m_linkedInternal.begin(); it != send; ++it) {
        ProjectItem *lib = projectInfo->findItem(QUrl::fromLocalFile(projectInfo->directory() + *it));
        if (!lib) {
            KMessageBox::error(nullptr, i18n("Do not know how to build \"%1\" (library does not exist in project).", *it));
            return false;
        }

        if (!lib->build(pol))
            return false;
    }

    // Build all children
    m_children.removeAll(static_cast<ProjectItem *>(nullptr));
    ProjectItemList::iterator cend = m_children.end();
    for (ProjectItemList::iterator it = m_children.begin(); it != cend; ++it) {
        if (!(*it)->build(pol))
            return false;
    }

    // Now build ourself
    ProcessOptions po;
    po.b_addToProject = false;
    po.setTargetFile(outputURL().toLocalFile());
    po.m_picID = microID();

    ProcessOptions::ProcessPath::MediaType typeTo = ProcessOptions::ProcessPath::Unknown;

    switch (outputType()) {
    case UnknownOutput:
        KMessageBox::error(nullptr, i18n("Do not know how to build \"%1\" (unknown output type).", name()));
        return false;

    case ProgramOutput:
        typeTo = ProcessOptions::ProcessPath::Program;
        break;

    case ObjectOutput:
        typeTo = ProcessOptions::ProcessPath::Object;
        break;

    case LibraryOutput:
        typeTo = ProcessOptions::ProcessPath::Library;
        break;
    }

    switch (type()) {
    case ProjectType:
        // Nothing to do
        return true;

    case FileType: {
        const QString fileName = url().toLocalFile();
        po.setInputFiles(QStringList(fileName));
        po.setProcessPath(ProcessOptions::ProcessPath::path(ProcessOptions::guessMediaType(fileName), typeTo));
        break;
    }
    case ProgramType:
    case LibraryType:
        // Build up a list of input urls
        QStringList inputFiles;

        // Link child objects
        m_children.removeAll(static_cast<ProjectItem *>(nullptr));
        ProjectItemList::iterator cend = m_children.end();
        for (ProjectItemList::iterator it = m_children.begin(); it != cend; ++it)
            inputFiles << (*it)->outputURL().toLocalFile();

        po.setInputFiles(inputFiles);
        po.setProcessPath(ProcessOptions::ProcessPath::path(ProcessOptions::ProcessPath::Object, typeTo));
        break;
    }

    po.m_hexFormat = hexFormatToString(hexFormat());
    po.m_bOutputMapFile = outputMapFile();
    po.m_libraryDir = libraryDir();
    po.m_linkerScript = linkerScript();
    po.m_linkOther = linkerOther();

    // Link against libraries
    QStringList::iterator lend = m_linkedInternal.end();
    for (QStringList::iterator it = m_linkedInternal.begin(); it != lend; ++it)
        po.m_linkLibraries += projectInfo->directory() + *it;
    lend = m_linkedExternal.end();
    for (QStringList::iterator it = m_linkedExternal.begin(); it != lend; ++it)
        po.m_linkLibraries += *it;

    // Save our working file (if open) and append to the build list
    Document *currentDoc = DocManager::self()->findDocument(url());
    if (currentDoc)
        currentDoc->fileSave();
    pol->append(po);

    return true;
}

void ProjectItem::upload(ProcessOptionsList *pol)
{
    build(pol);

    ProgrammerDlg *dlg = new ProgrammerDlg(microID(), static_cast<QWidget *>(KTechlab::self()));
    dlg->setObjectName("Programmer Dlg"_L1);

    const int accepted = dlg->exec();
    if (accepted != QDialog::Accepted) {
        dlg->deleteLater();
        return;
    }

    ProcessOptions po;
    dlg->initOptions(&po);
    po.b_addToProject = false;
    po.setInputFiles(QStringList(outputURL().toLocalFile()));
    po.setProcessPath(ProcessOptions::ProcessPath::Program_PIC);

    pol->append(po);

    dlg->deleteLater();
}

QDomElement ProjectItem::toDomElement(QDomDocument &doc, const QUrl &baseDirUrl) const
{
    QDomElement node = doc.createElement("item"_L1);

    node.setAttribute("type"_L1, typeToString());
    node.setAttribute("name"_L1, m_name);
    node.setAttribute("url"_L1, ::relativeUrl(baseDirUrl, m_url));

    node.appendChild(LinkerOptions::toDomElement(doc, baseDirUrl));
    node.appendChild(ProcessingOptions::toDomElement(doc, baseDirUrl));

    ProjectItemList::const_iterator end = m_children.end();
    for (ProjectItemList::const_iterator it = m_children.begin(); it != end; ++it) {
        if (*it)
            node.appendChild((*it)->toDomElement(doc, baseDirUrl));
    }

    return node;
}

QList<QUrl> ProjectItem::childOutputURLs(unsigned types, unsigned outputTypes) const
{
    QList<QUrl> urls;

    ProjectItemList::const_iterator end = m_children.end();
    for (ProjectItemList::const_iterator it = m_children.begin(); it != end; ++it) {
        if (!*it)
            continue;

        if (((*it)->type() & types) && ((*it)->outputType() & outputTypes))
            urls += (*it)->outputURL();

        urls += (*it)->childOutputURLs(types);
    }

    return urls;
}

ProjectItem *ProjectItem::findItem(const QUrl &url)
{
    if (this->url() == url)
        return this;

    ProjectItemList::const_iterator end = m_children.end();
    for (ProjectItemList::const_iterator it = m_children.begin(); it != end; ++it) {
        if (!*it)
            continue;

        ProjectItem *found = (*it)->findItem(url);
        if (found)
            return found;
    }

    return nullptr;
}

bool ProjectItem::closeOpenFiles()
{
    Document *doc = DocManager::self()->findDocument(m_url);
    if (doc && !doc->fileClose())
        return false;

    m_children.removeAll(static_cast<ProjectItem *>(nullptr));
    ProjectItemList::iterator end = m_children.end();
    for (ProjectItemList::iterator it = m_children.begin(); it != end; ++it) {
        if (!(*it)->closeOpenFiles())
            return false;
    }

    return true;
}

void ProjectItem::addFiles()
{
    const QList<QUrl> urls = KTechlab::self()->getFileURLs();
    for (const QUrl &url : urls)
        addFile(url);
}

void ProjectItem::addCurrentFile()
{
    Document *document = DocManager::self()->getFocusedDocument();
    if (!document)
        return;

    // If the file isn't saved yet, we must do that
    // before it is added to the project.
    if (document->url().isEmpty()) {
        document->fileSaveAs();
        // If the user pressed cancel then just give up,
        // otherwise the file can now be added.
    }

    if (!document->url().isEmpty())
        addFile(document->url());
}

void ProjectItem::addFile(const QUrl &url)
{
    if (url.isEmpty())
        return;

    m_children.removeAll(static_cast<ProjectItem *>(nullptr));
    ProjectItemList::iterator end = m_children.end();
    for (ProjectItemList::iterator it = m_children.begin(); it != end; ++it) {
        if ((*it)->type() == FileType && (*it)->url() == url)
            return;
    }

    ProjectItem *item = new ProjectItem(this, FileType, m_pProjectManager);
    item->setURL(url);
    addChild(item);
}

QString ProjectItem::typeToString() const
{
    switch (m_type) {
    case ProjectType:
        return "Project"_L1;

    case FileType:
        return "File"_L1;

    case ProgramType:
        return "Program"_L1;

    case LibraryType:
        return "Library"_L1;
    }
    return QString();
}

ProjectItem::Type ProjectItem::stringToType(const QString &type)
{
    if (type == "Project"_L1)
        return ProjectType;

    if (type == "File"_L1)
        return FileType;

    if (type == "Program"_L1)
        return ProgramType;

    if (type == "Library"_L1)
        return LibraryType;

    return FileType;
}

void ProjectItem::domElementToItem(const QDomElement &element, const QUrl &baseDirUrl)
{
    Type type = stringToType(element.attribute("type"_L1, QString()));
    QString name = element.attribute("name"_L1, QString());
    QUrl url = baseDirUrl.resolved(QUrl(element.attribute("url"_L1, QString())));

    ProjectItem *createdItem = new ProjectItem(this, type, m_pProjectManager);
    createdItem->setObjectName(name);
    createdItem->setURL(url);

    addChild(createdItem);

    QDomNode node = element.firstChild();
    while (!node.isNull()) {
        QDomElement childElement = node.toElement();
        if (!childElement.isNull()) {
            const QString tagName = childElement.tagName();

            if (tagName == "linker"_L1)
                createdItem->domElementToLinkerOptions(childElement, baseDirUrl);

            else if (tagName == "processing"_L1)
                createdItem->domElementToProcessingOptions(childElement, baseDirUrl);

            else if (tagName == "item"_L1)
                createdItem->domElementToItem(childElement, baseDirUrl);

            else
                qCCritical(KTL_LOG) << "Unrecognised element tag name: " << tagName;
        }

        node = node.nextSibling();
    }
}
// END class ProjectItem

// BEGIN class ProjectInfo
ProjectInfo::ProjectInfo(ProjectManager *projectManager)
    : ProjectItem(nullptr, ProjectItem::ProjectType, projectManager)
{
    m_microID = QString();
}

ProjectInfo::~ProjectInfo()
{
}

bool ProjectInfo::open(const QUrl &url)
{
    QScopedPointer<QFile> file;
    if (!url.isLocalFile()) {
        QScopedPointer<QTemporaryFile> downloadedFile(new QTemporaryFile());
        downloadedFile->open();
        KIO::FileCopyJob *job = KIO::file_copy(url, QUrl::fromLocalFile(downloadedFile->fileName()));
        KJobWidgets::setWindow(job, nullptr);
        if (!job->exec()) {
            KMessageBox::error(nullptr, job->errorString());
            return false;
        }
        file.reset(downloadedFile.take());
    } else {
        QScopedPointer<QFile> localFile(new QFile(url.toLocalFile()));
        if (!localFile->open(QIODevice::ReadOnly)) {
            KMessageBox::error(nullptr, i18n("Could not open %1 for reading", localFile->fileName()));
            return false;
        }
        file.reset(localFile.take());
    }

    m_url = url;

    QString xml;
    QTextStream textStream(file.data());
    while (!textStream.atEnd()) // was: eof()
        xml += textStream.readLine() + QLatin1Char('\n');

    QDomDocument doc("KTechlab"_L1);
    QString errorMessage;
    if (!doc.setContent(xml, &errorMessage)) {
        KMessageBox::error(nullptr, i18n("Could not parse XML:\n%1", errorMessage));
        return false;
    }

    QDomElement root = doc.documentElement();

    QDomNode node = root.firstChild();
    while (!node.isNull()) {
        QDomElement element = node.toElement();
        if (!element.isNull()) {
            const QString tagName = element.tagName();

            if (tagName == "linker"_L1)
                domElementToLinkerOptions(element, m_url);

            else if (tagName == "processing"_L1)
                domElementToProcessingOptions(element, m_url);

            else if (tagName == "file"_L1 || tagName == "item"_L1)
                domElementToItem(element, m_url);

            else
                qCWarning(KTL_LOG) << "Unrecognised element tag name: " << tagName;
        }

        node = node.nextSibling();
    }

    updateControlChildMicroIDs();
    return true;
}

bool ProjectInfo::save()
{
    QFile file(m_url.toLocalFile());
    if (file.open(QIODevice::WriteOnly) == false) {
        KMessageBox::error(nullptr, i18n("Project could not be saved to \"%1\"", file.fileName()), i18n("Saving Project"));
        return false;
    }

    QDomDocument doc("KTechlab"_L1);

    QDomElement root = doc.createElement("project"_L1);
    doc.appendChild(root);

    m_children.removeAll(static_cast<ProjectItem *>(nullptr));
    const QUrl baseDirUrl = m_url.adjusted(QUrl::RemoveFilename);
    ProjectItemList::const_iterator end = m_children.end();
    for (ProjectItemList::const_iterator it = m_children.begin(); it != end; ++it)
        root.appendChild((*it)->toDomElement(doc, baseDirUrl));

    QTextStream stream(&file);
    stream << doc.toString();
    file.close();

    {
        KRecentFilesAction *rfa = static_cast<KRecentFilesAction *>(KTechlab::self()->actionByName("project_open_recent"_L1));
        if (rfa) {
            KSharedConfigPtr config = KSharedConfig::openConfig();
            rfa->addUrl(m_url);
            rfa->saveEntries(config->group("Recent Projects"_L1));
            config->sync();
        } else {
            qCWarning(KTL_LOG) << "there is no project_open_recent action in application";
        }
    }

    return true;
}

bool ProjectInfo::saveAndClose()
{
    if (!save())
        return false;

    if (!closeOpenFiles())
        return false;

    return true;
}
// END class ProjectInfo

// BEGIN class ProjectManager
ProjectManager *ProjectManager::m_pSelf = nullptr;

ProjectManager *ProjectManager::self(KateMDI::ToolView *parent)
{
    if (!m_pSelf) {
        assert(parent);
        m_pSelf = new ProjectManager(parent);
        m_pSelf->setObjectName("Project Manager");
    }
    return m_pSelf;
}

ProjectManager::ProjectManager(KateMDI::ToolView *parent)
    : ItemSelector(parent)
    , m_pCurrentProject(nullptr)
{
    setWhatsThis(i18n("Displays the list of files in the project.\nTo open or close a project, use the \"Project\" menu. Right click on a file to remove it from the project"));

    setListCaption(i18n("File"));
    setWindowTitle(i18n("Project Manager"));

    connect(this, SIGNAL(itemClicked(QTreeWidgetItem *, int)), this, SLOT(slotItemClicked(QTreeWidgetItem *, int)));
}

ProjectManager::~ProjectManager()
{
}

void ProjectManager::slotNewProject()
{
    if (!slotCloseProject())
        return;

    NewProjectDlg *newProjectDlg = new NewProjectDlg(this);
    const int accepted = newProjectDlg->exec();

    if (accepted == QDialog::Accepted) {
        m_pCurrentProject = new ProjectInfo(this);
        m_pCurrentProject->setObjectName(newProjectDlg->projectName());
        m_pCurrentProject->setURL(QUrl::fromLocalFile(
            newProjectDlg->location() + m_pCurrentProject->name().toLower() + QLatin1StringView(".ktechlab")));

        QDir dir;
        if (!dir.mkdir(m_pCurrentProject->directory()))
            qCDebug(KTL_LOG) << "Error in creating directory " << m_pCurrentProject->directory();

        m_pCurrentProject->save();
        updateActions();

        Q_EMIT projectCreated();
    }

    delete newProjectDlg;
}

void ProjectManager::slotProjectOptions()
{
}

void ProjectManager::slotOpenProject()
{
    QString filter;
    filter = QLatin1StringView("*.ktechlab|%1 (*.ktechlab)\n*|%2").arg(i18n("KTechlab Project")).arg(i18n("All Files"));

    QUrl url = QFileDialog::getOpenFileUrl(this, i18n("Open Location"), QUrl(), filter);

    if (url.isEmpty())
        return;

    slotOpenProject(url);
}

void ProjectManager::slotOpenProject(const QUrl &url)
{
    if (m_pCurrentProject && m_pCurrentProject->url() == url)
        return;

    if (!slotCloseProject())
        return;

    m_pCurrentProject = new ProjectInfo(this);

    if (!m_pCurrentProject->open(url)) {
        m_pCurrentProject->deleteLater();
        m_pCurrentProject = nullptr;
        return;
    }
    {
        KRecentFilesAction *rfa = static_cast<KRecentFilesAction *>(KTechlab::self()->actionByName("project_open_recent"_L1));
        if (rfa) {
            KSharedConfigPtr config = KSharedConfig::openConfig();
            rfa->addUrl(m_pCurrentProject->url());
            rfa->saveEntries(config->group("Recent Projects"_L1));
            config->sync();
        } else {
            qCWarning(KTL_LOG) << "there is no project_open_recent action in application";
        }
    }

    if (KTLConfig::raiseItemSelectors())
        KTechlab::self()->showToolView(KTechlab::self()->toolView(toolViewIdentifier()));

    updateActions();
    Q_EMIT projectOpened();
}

bool ProjectManager::slotCloseProject()
{
    if (!m_pCurrentProject)
        return true;

    if (!m_pCurrentProject->saveAndClose())
        return false;

    m_pCurrentProject->deleteLater();
    m_pCurrentProject = nullptr;
    updateActions();
    Q_EMIT projectClosed();
    return true;
}

void ProjectManager::slotCreateSubproject()
{
    if (!currentProject())
        return;

    CreateSubprojectDlg *dlg = new CreateSubprojectDlg(this);
    const int accepted = dlg->exec();

    if (accepted == QDialog::Accepted) {
        ProjectItem::Type type = ProjectItem::ProgramType;
        switch (dlg->type()) {
        case CreateSubprojectDlg::ProgramType:
            type = ProjectItem::ProgramType;
            break;

        case CreateSubprojectDlg::LibraryType:
            type = ProjectItem::LibraryType;
            break;
        }

        ProjectItem *subproject = new ProjectItem(currentProject(), type, this);
        subproject->setURL(QUrl::fromLocalFile(dlg->targetFile()));

        currentProject()->addChild(subproject);
        currentProject()->save();

        Q_EMIT subprojectCreated();
    }

    delete dlg;
}

void ProjectManager::updateActions()
{
    bool projectIsOpen = m_pCurrentProject;

    KTechlab::self()->actionByName("project_create_subproject"_L1)->setEnabled(projectIsOpen);
    KTechlab::self()->actionByName("project_export_makefile"_L1)->setEnabled(projectIsOpen);
    KTechlab::self()->actionByName("subproject_add_existing_file"_L1)->setEnabled(projectIsOpen);
    KTechlab::self()->actionByName("subproject_add_current_file"_L1)->setEnabled(projectIsOpen);
    // 	KTechlab::self()->actionByName("project_options"_L1)->setEnabled( projectIsOpen );
    KTechlab::self()->actionByName("project_close"_L1)->setEnabled(projectIsOpen);
    KTechlab::self()->actionByName("project_add_existing_file"_L1)->setEnabled(projectIsOpen);
    KTechlab::self()->actionByName("project_add_current_file"_L1)->setEnabled(projectIsOpen);
}

void ProjectManager::slotAddFile()
{
    if (!currentProject())
        return;

    currentProject()->addFiles();
    Q_EMIT filesAdded();
}

void ProjectManager::slotAddCurrentFile()
{
    if (!currentProject())
        return;
    currentProject()->addCurrentFile();
    Q_EMIT filesAdded();
}

void ProjectManager::slotSubprojectAddExistingFile()
{
    ILVItem *currentItem = dynamic_cast<ILVItem *>(selectedItem());
    if (!currentItem || !currentItem->projectItem())
        return;

    currentItem->projectItem()->addFiles();
    Q_EMIT filesAdded();
}

void ProjectManager::slotSubprojectAddCurrentFile()
{
    ILVItem *currentItem = dynamic_cast<ILVItem *>(selectedItem());
    if (!currentItem || !currentItem->projectItem())
        return;

    currentItem->projectItem()->addCurrentFile();
    Q_EMIT filesAdded();
}

void ProjectManager::slotItemBuild()
{
    ILVItem *currentItem = dynamic_cast<ILVItem *>(selectedItem());
    if (!currentItem || !currentItem->projectItem())
        return;

    ProcessOptionsList pol;
    currentItem->projectItem()->build(&pol);
    LanguageManager::self()->compile(pol);
}

void ProjectManager::slotItemUpload()
{
    ILVItem *currentItem = dynamic_cast<ILVItem *>(selectedItem());
    if (!currentItem || !currentItem->projectItem())
        return;

    ProcessOptionsList pol;
    currentItem->projectItem()->upload(&pol);
    LanguageManager::self()->compile(pol);
}

void ProjectManager::slotRemoveSelected()
{
    ILVItem *currentItem = dynamic_cast<ILVItem *>(selectedItem());
    if (!currentItem)
        return;

    int choice = KMessageBox::questionTwoActions(this, i18n("Do you really want to remove \"%1\"?", currentItem->text(0)), i18n("Remove Project File?"), KGuiItem(i18n("Remove")), KGuiItem(i18n("Cancel")));

    if (choice == QMessageBox::No)
        return;

    currentItem->projectItem()->deleteLater();
    Q_EMIT filesRemoved();
}

void ProjectManager::slotExportToMakefile()
{
}

void ProjectManager::slotSubprojectLinkerOptions()
{
    ILVItem *currentItem = dynamic_cast<ILVItem *>(selectedItem());
    if (!currentItem || !currentItem->projectItem())
        return;

    LinkerOptionsDlg *dlg = new LinkerOptionsDlg(currentItem->projectItem(), this);
    dlg->exec();
    currentProject()->save();

    // The dialog sets the options for us if it was accepted, so we don't need to do anything
    delete dlg;
}

void ProjectManager::slotItemProcessingOptions()
{
    ILVItem *currentItem = dynamic_cast<ILVItem *>(selectedItem());
    if (!currentItem || !currentItem->projectItem())
        return;

    ProcessingOptionsDlg *dlg = new ProcessingOptionsDlg(currentItem->projectItem(), this);
    dlg->exec();
    currentProject()->save();

    // The dialog sets the options for us if it was accepted, so we don't need to do anything
    delete dlg;
}

void ProjectManager::slotItemClicked(QTreeWidgetItem *item, int)
{
    ILVItem *ilvItem = dynamic_cast<ILVItem *>(item);
    if (!ilvItem)
        return;

    ProjectItem *projectItem = ilvItem->projectItem();
    if (!projectItem || projectItem->type() != ProjectItem::FileType)
        return;

    DocManager::self()->openURL(projectItem->url());
}

void ProjectManager::slotContextMenuRequested(const QPoint &pos)
{
    QTreeWidgetItem *item = itemAt(pos);
    QString popupName;
    ILVItem *ilvItem = dynamic_cast<ILVItem *>(item);
    QAction *linkerOptionsAct = KTechlab::self()->actionByName("project_item_linker_options"_L1);
    linkerOptionsAct->setEnabled(false);

    if (!m_pCurrentProject) {
        popupName = "project_none_popup"_L1;

    } else if (!ilvItem) {
        popupName = "project_blank_popup"_L1;

    } else {
        ProcessOptions::ProcessPath::MediaType mediaType = ProcessOptions::guessMediaType(ilvItem->projectItem()->url().toLocalFile());

        switch (ilvItem->projectItem()->type()) {
        case ProjectItem::FileType:
            if (mediaType == ProcessOptions::ProcessPath::Unknown)
                popupName = "project_file_other_popup"_L1;
            else
                popupName = "project_file_popup"_L1;
            break;

        case ProjectItem::ProgramType:
            popupName = "project_program_popup"_L1;
            break;

        case ProjectItem::LibraryType:
            popupName = "project_library_popup"_L1;
            break;

        case ProjectItem::ProjectType:
            return;
        }
        switch (ilvItem->projectItem()->outputType()) {
        case ProjectItem::ProgramOutput:
            linkerOptionsAct->setEnabled(true);
            break;

        case ProjectItem::ObjectOutput:
        case ProjectItem::LibraryOutput:
        case ProjectItem::UnknownOutput:
            linkerOptionsAct->setEnabled(false);
            break;
        }

        // Only have linking options for SDCC files
        linkerOptionsAct->setEnabled(mediaType == ProcessOptions::ProcessPath::C);
    }

    bool haveFocusedDocument = DocManager::self()->getFocusedDocument();
    KTechlab::self()->actionByName(QLatin1StringView("subproject_add_current_file"))->setEnabled(haveFocusedDocument);
    KTechlab::self()->actionByName(QLatin1StringView("project_add_current_file"))->setEnabled(haveFocusedDocument);

    QMenu *pop = static_cast<QMenu *>(KTechlab::self()->factory()->container(popupName, KTechlab::self()));
    if (pop) {
        QPoint globalPos = mapToGlobal(pos);
        pop->popup(globalPos);
    }
}
// END class ProjectManager

#include "moc_projectmanager.cpp"
