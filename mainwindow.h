#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QtCore>
#include <QtWidgets>

#include "interfaces/testinterface.h"
#include "interfaces/fileimportinterface.h"
#include "interfaces/viewerinterface.h"
#include "interfaces/projectinterface.h"
#include "interfaces/dataaccessorinterface.h"
#include "interfaces/dataproviderinterface.h"

#ifdef QT_SQL
#include <interfaces/databaseinterface.h>
#endif

namespace Ui {
class MainWindow;
}

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    explicit MainWindow(QWidget *parent = nullptr);
    ~MainWindow();

private slots:
    void on_pushButton_clicked();

private:
    Ui::MainWindow *ui;

    QList<std::shared_ptr<FileImportInterface> >_fileImporters;
    QList<std::shared_ptr<ViewerInterface> >_viewers;
    QList<std::shared_ptr<ProjectInterface> >_projectVisitors;
    QList<SpDataAccessorInterface> _dataAccessors;
    QList<SpDataProviderInterface> _dataProviders;
#ifdef QT_SQL
    QList<std::shared_ptr<DatabaseInterface> > _databaseVisitors;
#endif

    void loadPlugins();
};

#endif // MAINWINDOW_H
