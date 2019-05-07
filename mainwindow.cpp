#include "mainwindow.h"
#include "ui_mainwindow.h"

#include <viewer.h>

MainWindow::MainWindow(QWidget *parent) :
    QMainWindow(parent),
    ui(new Ui::MainWindow)
{
    ui->setupUi(this);
    loadPlugins();
}

MainWindow::~MainWindow()
{
    delete ui;
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
            auto databaseVisitor = qobject_cast<DatabaseInterface *>(plugin);
            if (databaseVisitor) {
                _databaseVisitors.push_back(std::shared_ptr<DatabaseInterface>(databaseVisitor));
            }
#endif
        }
    }
}

void MainWindow::on_pushButton_clicked()
{
#ifdef QT_SQL
    auto filePath = QFileDialog::getOpenFileName(this, tr("打开"), QString(), "数据库文件 (*.db)");

    if (QFile::exists(filePath)) {
        QSqlDatabase db = QSqlDatabase::addDatabase("QSQLITE", QUuid::createUuid().toString());
        db.setDatabaseName(filePath);
        if (!db.open()) {
            return;
        }
        for (auto visitor : _databaseVisitors) {
            visitor->setDatabase(db);
        }

        for (auto provider: _dataProviders) {
            if (provider->type() == QString("sqlite")) {
                for (auto accessor: _dataAccessors) {
                    accessor->setDataProvider(provider);
                }
            }
        }

        for (auto viewer : _viewers) {
            ui->tabWidget->addTab(new ViewerWindow(viewer->create(nullptr), this), viewer->name() + " - " + filePath);
        }
    }
#endif
}
