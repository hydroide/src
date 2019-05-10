#if _MSC_VER >= 1600
#pragma execution_character_set("utf-8")
#endif
#include "mainwindow.h"
#include "ui_mainwindow.h"

#include <viewer.h>

MainWindow::MainWindow(QWidget *parent) :
    QMainWindow(parent),
    ui(new Ui::MainWindow)
{
    ui->setupUi(this);

    connect(ui->tabWidget, SIGNAL(tabCloseRequested(int)),
            this, SLOT(closeTab(int)));

    loadPlugins();
    loadViewers();
}

MainWindow::~MainWindow()
{
    delete ui;
}

bool MainWindow::closeTab(const int& index)
{
    if (index == -1) {
        return true;
    }
    QWidget* tabItem = ui->tabWidget->widget(index);

    if (tabItem->close()) // 判断保存
    {
        ui->tabWidget->removeTab(index);

        tabItem->deleteLater();
        return true;
    }
    return false;
}

void MainWindow::loadPlugins()
{
    QDir pluginsDir(qApp->applicationDirPath());
#if defined(Q_OS_WIN)
    if (pluginsDir.dirName().toLower() == "debug" || pluginsDir.dirName().toLower() == "release")
        pluginsDir.cdUp();
#elif defined(Q_OS_MAC)
    if (pluginsDir.dirName() == "MacOS") {
        pluginsDir.cdUp();
        pluginsDir.cdUp();
        pluginsDir.cdUp();
    }
#endif
    pluginsDir.cd("plugins");
    for (auto fileName: pluginsDir.entryList(QDir::Files)) {
        QPluginLoader pluginLoader(pluginsDir.absoluteFilePath(fileName));
        QObject *plugin = pluginLoader.instance();
        if (plugin) {
            auto fileImporter = qobject_cast<FileImportInterface *>(plugin);
            if (fileImporter) {
                _fileImporters.push_back(std::shared_ptr<FileImportInterface>(fileImporter));
            }
            auto viewer = qobject_cast<ViewerInterface *>(plugin);
            if (viewer) {
                _viewers.push_back(std::shared_ptr<ViewerInterface>(viewer));
            }
            auto projectVisitor = qobject_cast<ProjectInterface *>(plugin);
            if (projectVisitor) {
                _projectVisitors.push_back(std::shared_ptr<ProjectInterface>(projectVisitor));
            }
            auto dataAccessor = qobject_cast<DataAccessorInterface *>(plugin);
            if (dataAccessor) {
                _dataAccessors.push_back(SpDataAccessorInterface(dataAccessor));
            }
            auto dataProvider = qobject_cast<DataProviderInterface *>(plugin);
            if (dataProvider) {
                _dataProviders.push_back(SpDataProviderInterface(dataProvider));
            }
#ifdef QT_SQL
            auto databasePlugin = qobject_cast<DatabaseInterface *>(plugin);
            if (databasePlugin) {
                _databasePlugins.push_back(std::shared_ptr<DatabaseInterface>(databasePlugin));
            }
#endif
        }
    }
}

void MainWindow::loadViewers()
{
    for (auto viewer: _viewers) {
        ui->mainToolBar->addAction(viewer->name(), [this, viewer]() {
            auto id = ui->tabWidget->addTab(viewer->create(this), viewer->name());
            ui->tabWidget->setCurrentIndex(id);
        });
    }
}

void MainWindow::on_actionOpen_triggered()
{
#ifdef QT_SQL
    auto filePath = QFileDialog::getOpenFileName(this, tr("打开"), QString(), "SQLite 数据库文件 (*.db)");

    if (!QFile::exists(filePath)) {
        return;
    }
    QSqlDatabase db = QSqlDatabase::addDatabase("QSQLITE", QUuid::createUuid().toString());
    db.setDatabaseName(filePath);
    if (!db.open()) {
        return;
    }
    for (auto plugin: _databasePlugins) {
        plugin->setDatabase(db);
    }
    for (auto provider: _dataProviders) {
        if (provider->type() == QString("sqlite")) {
            for (auto accessor: _dataAccessors) {
                accessor->setDataProvider(provider);
            }
        }
    }
#endif
}

void MainWindow::on_actionCreate_triggered()
{
#ifdef QT_SQL
    auto filePath = QFileDialog::getSaveFileName(this, tr("创建"), QString(), "SQLite 数据库文件 (*.db)");

    if (QFile::exists(filePath)) {
        auto button = QMessageBox::question(this, tr("重要"), tr("文件%1中的内容将全部删除，是否继续？").arg(filePath));
        if (button == QMessageBox::No) {
            return;
        }
    }
    QSqlDatabase db = QSqlDatabase::addDatabase("QSQLITE", QUuid::createUuid().toString());
    db.setDatabaseName(filePath);
    if (!db.open()) {
        return;
    }

    for (auto plugin : _databasePlugins) {
        plugin->setDatabase(db);
        plugin->initDatabase();
    }
    for (auto provider: _dataProviders) {
        if (provider->type() == QString("sqlite")) {
            for (auto accessor: _dataAccessors) {
                accessor->setDataProvider(provider);
            }
        }
    }
#endif
}

void MainWindow::on_actionImport_triggered()
{
    QStringList supportedTypes;
    QMap<QString, std::function<void (const QString &)>> operations;

    for (auto importer: _fileImporters) {
        QStringList formattedExtentions;
        auto extensions = importer->extensions();
        for (auto extension: extensions) {
            operations[extension.toLower()] = [=](const QString &filename) {
                importer->import(filename);
            };
            formattedExtentions << QString("*.%1").arg(extension);
        }
        supportedTypes << QString("%1 (%2)").arg(importer->description()).arg(formattedExtentions.join(" "));
    }

    auto filenames = QFileDialog::getOpenFileNames(
                this,
                tr("导入数据文件"),
                QString(),
                supportedTypes.join(";;"));

    if (filenames.isEmpty())
    {
        return;
    }
    try
    {
        for (const auto & filename: filenames)
        {
            QFileInfo fi(filename);
            if (!fi.exists())
            {
                QMessageBox::critical(this, tr("错误"), tr("文件不存在：%1").arg(filename));
                continue;
            }
            auto ext = fi.suffix().toLower();
            if (operations.contains(ext)) {
                operations[ext](filename);
            } else {
                QMessageBox::critical(this, tr("错误"), tr("不认识这种文件格式。"));
                return;
            }
        }
        QMessageBox::information(this, tr("导入完成"), tr("导入完成"));
    }
    catch (std::exception e) {
        QMessageBox::critical(this, tr("错误"), tr("导入文件发生错误，临时: %1").arg(e.what()));
    }
}
