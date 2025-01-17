#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QPushButton>
#include <QVBoxLayout>
#include <QLabel>
#include <QNetworkAccessManager>
#include <QNetworkRequest>
#include <QNetworkReply>
#include <QUrl>
#include <QUrlQuery>
#include <QRegularExpression>
#include <QtCharts/QChart>
#include <QtCharts/QChartView>
#include <QtCharts/QLineSeries>
#include <QtCharts/QValueAxis>
#include <QtCharts/QDateTimeAxis>
#include <QLineEdit>
#include <QDateTime>
#include <QVector>
#include <QToolButton> 

class MainWindow : public QMainWindow {
    Q_OBJECT

public:
    explicit MainWindow(QWidget *parent = nullptr);
    ~MainWindow();

private slots:
    void sendRequest();
    void sendRequestsToGraphs();
    void handleReplyToLast(QNetworkReply *reply);
    void handleReplyToGraphs(QNetworkReply *reply, int index);

private:
    QLineEdit *startDateEdit;
    QLineEdit *endDateEdit;
    void clearGraphs();
    void showGraph(const QVector<QDateTime> &x, const QVector<double> &y, const QString &title);

    QToolButton *updateButton;
    QNetworkAccessManager *networkManager;
    QLabel *resultLabel;
    QLabel *graphStatusLabel;
    QVector<QtCharts::QChartView*> chartViews;
    QVBoxLayout *chartLayout;
    QWidget *chartWidget;
    int repliesPending;

    struct ReplyData {
        QVector<QDateTime> x;
        QVector<double> y;
    };
    QVector<ReplyData> repliesData;

    QTimer *timer;
};

#endif
