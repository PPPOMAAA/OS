#include "mainwindow.h"
#include <QNetworkReply>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <QLineEdit>
#include <QVBoxLayout>
#include <QPushButton>
#include <QRegularExpression>
#include <QtCharts/QLineSeries>
#include <QtCharts/QValueAxis>
#include <QtCharts/QDateTimeAxis>
#include <QDateTime>
#include <QDebug>
#include <QMessageBox>
#include <QScrollArea>
#include <QGroupBox>
#include <QLineEdit>
#include <QHBoxLayout>
#include <QTimer>

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent), repliesPending(0) {

    setWindowTitle("Temperature manager");

    QFile file(":/styles/styles.qss");
    if (file.open(QFile::ReadOnly)) {
        QString styleSheet = QLatin1String(file.readAll());
        this->setStyleSheet(styleSheet);
        file.close();
    } else {
        qDebug() << "Не удалось загрузить файл стилей";
    }

    auto *mainWidget = new QWidget;
    auto *mainLayout = new QVBoxLayout;

    QGroupBox *singleRequestGroup = new QGroupBox("Current Temperature");
    QHBoxLayout *singleRequestLayout = new QHBoxLayout;

    resultLabel = new QLabel("");
    QFont font = resultLabel->font();
    font.setPointSize(24);
    resultLabel->setFont(font);

    updateButton = new QToolButton();
    updateButton->setIcon(QIcon(":/icons/refresh.png"));
    updateButton->resize(64, 64);
    updateButton->setToolTip("Update Temperature");

    singleRequestLayout->addWidget(resultLabel);
    singleRequestLayout->addWidget(updateButton);

    singleRequestGroup->setLayout(singleRequestLayout);
    mainLayout->addWidget(singleRequestGroup);


    QGroupBox *multiRequestGroup = new QGroupBox("Temperature Statistics");
    QVBoxLayout *multiRequestLayout = new QVBoxLayout;

    auto *dateInputLayout = new QHBoxLayout;
    startDateEdit = new QLineEdit;
    startDateEdit->setPlaceholderText("Start date (YYYY-MM-DD HH:mm:ss)");
    endDateEdit = new QLineEdit;
    endDateEdit->setPlaceholderText("End date (YYYY-MM-DD HH:mm:ss)");
    dateInputLayout->addWidget(new QLabel("Start Date:"));
    dateInputLayout->addWidget(startDateEdit);
    dateInputLayout->addWidget(new QLabel("End Date:"));
    dateInputLayout->addWidget(endDateEdit);
    multiRequestLayout->addLayout(dateInputLayout);

    auto *multiRequestButton = new QPushButton("Get Statistics");
    graphStatusLabel = new QLabel("");
    multiRequestLayout->addWidget(multiRequestButton);
    multiRequestLayout->addWidget(graphStatusLabel);
    multiRequestGroup->setLayout(multiRequestLayout);
    mainLayout->addWidget(multiRequestGroup);

    QScrollArea *chartScrollArea = new QScrollArea;
    chartScrollArea->setWidgetResizable(true);
    chartWidget = new QWidget;
    chartLayout = new QVBoxLayout;
    chartWidget->setLayout(chartLayout);
    chartScrollArea->setWidget(chartWidget);
    mainLayout->addWidget(chartScrollArea);

    mainWidget->setLayout(mainLayout);

    setCentralWidget(mainWidget);

    networkManager = new QNetworkAccessManager(this);

    connect(updateButton, &QToolButton::clicked, this, &MainWindow::sendRequest);
    connect(multiRequestButton, &QPushButton::clicked, this, &MainWindow::sendRequestsToGraphs);

    timer = new QTimer(this);
    connect(timer, &QTimer::timeout, this, &MainWindow::sendRequest);

    sendRequest();
    timer->start(5000);
}

MainWindow::~MainWindow(){
    timer->stop();
    delete timer;
}


void MainWindow::sendRequestsToGraphs() {
    QString startDateStr = startDateEdit->text().trimmed();
    QString endDateStr = endDateEdit->text().trimmed();

    QDateTime startDate = QDateTime::fromString(startDateStr, "yyyy-MM-dd HH:mm:ss");
    QDateTime endDate = QDateTime::fromString(endDateStr, "yyyy-MM-dd HH:mm:ss");

    if (!startDate.isValid()) {
        startDate = QDateTime::fromString("1970-01-01 00:00:00", "yyyy-MM-dd HH:mm:ss");
        startDateEdit->setText("1970-01-01 00:00:00");
    }
    if (!endDate.isValid()) {
        endDate = QDateTime::fromString("2026-01-01 00:00:00", "yyyy-MM-dd HH:mm:ss");
        endDateEdit->setText("2026-01-01 00:00:00");
    }

    if (startDate >= endDate) {
        QMessageBox::warning(this, "Invalid Dates", "Start date must be before end date.");
        return;
    }

    QStringList tables = {"data_current", "data_hour", "data_day"};
    clearGraphs();
    repliesPending = tables.size();

    repliesData.clear();
    repliesData.resize(tables.size());

    for (int i = 0; i < tables.size(); ++i) {
        const QString &table = tables[i];
        QUrl url("http://mysitehost:8080/data");
        QUrlQuery query;
        query.addQueryItem("start", startDate.toString("yyyy-MM-dd HH:mm:ss"));
        query.addQueryItem("end", endDate.toString("yyyy-MM-dd HH:mm:ss"));
        query.addQueryItem("table", table);
        url.setQuery(query);

        QNetworkRequest request(url);
        QNetworkReply *reply = networkManager->get(request);

        connect(reply, &QNetworkReply::finished, this, [this, reply, i]() {
            handleReplyToGraphs(reply, i);
        });
        connect(reply, static_cast<void (QNetworkReply::*)(QNetworkReply::NetworkError)>(&QNetworkReply::error),
                this, [this, reply](){
            qDebug() << "Error: " << reply->errorString();
            reply->deleteLater();
            if (--repliesPending == 0) {
                graphStatusLabel->setText("Some graphs failed to load.");
            }
        });
    }
    graphStatusLabel->setText("Loading...");
}

void MainWindow::handleReplyToGraphs(QNetworkReply *reply, int index) {
    if (reply->error() == QNetworkReply::NoError) {
        QString response = reply->readAll();
        QVector<QDateTime> x;
        QVector<double> y;

        QStringList lines = response.split('\n', QString::SkipEmptyParts);

        for (const QString &line : lines) {
            QRegularExpression regex(R"(Date:\s*([\d\- :]+),\s*Temperature:\s*([\d\.\-]+))");
            QRegularExpressionMatch match = regex.match(line);
            if (match.hasMatch()) {
                QDateTime dateTime = QDateTime::fromString(match.captured(1), "yyyy-MM-dd HH:mm:ss");
                if (dateTime.isValid()) {
                    x.append(dateTime);
                    y.append(match.captured(2).toDouble());
                } else {
                    qDebug() << "Invalid date format: " << match.captured(1);
                }
            }
        }
        repliesData[index] = {x, y};
    } else {
        qDebug() << "Network error: " << reply->errorString();
    }

    reply->deleteLater();

    if (--repliesPending == 0) {
        QStringList titles = {"Current Data", "Hourly Data", "Daily Data"};
        for (int i = 0; i < repliesData.size(); ++i) {
            if (!repliesData[i].x.isEmpty() && !repliesData[i].y.isEmpty()) {
                showGraph(repliesData[i].x, repliesData[i].y, titles[i]);
            }
        }
        graphStatusLabel->setText("All graphs are processed!");
    }
}

void MainWindow::clearGraphs() {
    if (chartLayout) {
        QLayoutItem *child;
        while ((child = chartLayout->takeAt(0)) != nullptr) {
            QWidget *widget = child->widget();
            if (widget) {
                chartLayout->removeWidget(widget);
                delete widget;
            }
            delete child;
        }
    }
    chartViews.clear();
}

void MainWindow::showGraph(const QVector<QDateTime> &x, const QVector<double> &y, const QString &title) {
    if (x.isEmpty() || y.isEmpty() || x.size() != y.size()) {
        qDebug() << "Invalid data for graph: " << title;
        return;
    }

    auto *chart = new QtCharts::QChart();
    auto *series = new QtCharts::QLineSeries();

    QPen pen(QColor(0, 128, 0));
    pen.setWidth(2);
    series->setPen(pen);

    for (int i = 0; i < x.size(); ++i) {
        series->append(x[i].toMSecsSinceEpoch(), y[i]);
    }

    series->setName(title);
    chart->addSeries(series);

    auto *axisX = new QtCharts::QDateTimeAxis();
    axisX->setFormat("yyyy-MM-dd HH:mm:ss");
    axisX->setTitleText("Date");
    chart->addAxis(axisX, Qt::AlignBottom);
    series->attachAxis(axisX);

    auto *axisY = new QtCharts::QValueAxis;
    axisY->setTitleText("Temperature (°C)");
    chart->addAxis(axisY, Qt::AlignLeft);
    series->attachAxis(axisY);

    chart->setTitle("Temperature Graph: " + title);
    chart->legend()->setVisible(true);
    chart->legend()->setAlignment(Qt::AlignBottom);

    auto *chartView = new QtCharts::QChartView(chart);
    chartView->setRenderHint(QPainter::Antialiasing);
    chartView->setMinimumHeight(300);
    chartViews.append(chartView);
    chartLayout->addWidget(chartView);
}


void MainWindow::sendRequest() {
    QUrl url("http://mysitehost:8080/data");
    QUrlQuery query;
    query.addQueryItem("table", "data_current");
    query.addQueryItem("last", "true");
    url.setQuery(query);

    QNetworkRequest request(url);

    QNetworkReply *reply = networkManager->get(request);

    connect(reply, &QNetworkReply::finished, this, [this, reply]() {
        handleReplyToLast(reply);
    });
    connect(reply, static_cast<void (QNetworkReply::*)(QNetworkReply::NetworkError)>(&QNetworkReply::error),
            this, [this, reply](QNetworkReply::NetworkError err) {
        qDebug() << "Network error: " << err;
        qDebug() << "Network error string: " << reply->errorString();
        reply->deleteLater();
    });
}

void MainWindow::handleReplyToLast(QNetworkReply *reply) {
    if (reply->error() == QNetworkReply::NoError) {
        QByteArray response = reply->readAll();
        QString responseText = QString::fromUtf8(response);

        QRegularExpression regex(R"(\bTemperature:\s*(-?\d+(\.\d+)?))");
        QRegularExpressionMatch match = regex.match(responseText);

        if (match.hasMatch()) {
            QString temperature = match.captured(1);
            resultLabel->setText(QString("Temperature: %1").arg(temperature));
        } else {
            resultLabel->setText("Temperature not found in response.");
        }

    } else {
        resultLabel->setText("Error: " + reply->errorString());
    }
    reply->deleteLater();
}
