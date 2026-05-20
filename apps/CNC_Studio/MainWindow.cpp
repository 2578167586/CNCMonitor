#include "MainWindow.h"
#include "MonitorPage.h"
#include "SimulatorPage.h"

#include <QApplication>
#include <QFile>
#include <QFrame>
#include <QHBoxLayout>
#include <QLabel>
#include <QPushButton>
#include <QStackedWidget>
#include <QVBoxLayout>

namespace DncScada {

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
{
    buildUi();
    switchPage(0);
}

void MainWindow::buildUi()
{
    setWindowTitle(QStringLiteral("CNC Studio"));
    setMinimumSize(960, 600);

    // Apply style.qss
    QFile styleFile(QApplication::applicationDirPath() + QStringLiteral("/../apps/CNC_Studio/theme/style.qss"));
    if (!styleFile.exists()) {
        styleFile.setFileName(QStringLiteral("apps/CNC_Studio/theme/style.qss"));
    }
    if (styleFile.open(QFile::ReadOnly)) {
        setStyleSheet(QString::fromUtf8(styleFile.readAll()));
        styleFile.close();
    }

    QWidget *root = new QWidget(this);
    setCentralWidget(root);
    QVBoxLayout *rootLayout = new QVBoxLayout(root);
    rootLayout->setContentsMargins(0, 0, 0, 0);
    rootLayout->setSpacing(0);

    // === Title Bar ===
    QWidget *titleBar = new QWidget(root);
    titleBar->setProperty("cssClass", "titleBar");
    titleBar->setFixedHeight(40);
    QHBoxLayout *titleLayout = new QHBoxLayout(titleBar);
    titleLayout->setContentsMargins(16, 0, 16, 0);

    QPushButton *hamburgerBtn = new QPushButton(QStringLiteral("☰"), titleBar);
    hamburgerBtn->setProperty("cssClass", "navBtn");
    hamburgerBtn->setFixedSize(32, 32);
    hamburgerBtn->setCursor(Qt::PointingHandCursor);
    connect(hamburgerBtn, &QPushButton::clicked, this, [this]() {
        sidebarExpanded = !sidebarExpanded;
        int targetWidth = sidebarExpanded ? sidebarExpandedWidth : sidebarCollapsedWidth;
        sidebar->setMaximumWidth(targetWidth);
        sidebar->setMinimumWidth(targetWidth);
    });
    titleLayout->addWidget(hamburgerBtn);

    titleLabel = new QLabel(QStringLiteral("CNC Studio"), titleBar);
    QFont titleFont = titleLabel->font();
    titleFont.setPointSize(14);
    titleFont.setBold(true);
    titleLabel->setFont(titleFont);
    titleLabel->setStyleSheet("color: #e6edf3; background: transparent;");
    titleLayout->addWidget(titleLabel);
    titleLayout->addStretch();

    simStatusLabel = new QLabel(QStringLiteral("● Sim Idle"), titleBar);
    simStatusLabel->setStyleSheet("color: #8b949e; background: transparent; font-size: 11px;");
    titleLayout->addWidget(simStatusLabel);

    rootLayout->addWidget(titleBar);

    // === Body (sidebar + content) ===
    QHBoxLayout *bodyLayout = new QHBoxLayout;
    bodyLayout->setContentsMargins(0, 0, 0, 0);
    bodyLayout->setSpacing(0);

    // Sidebar
    sidebar = new QWidget(root);
    sidebar->setProperty("cssClass", "sidebar");
    sidebar->setFixedWidth(sidebarCollapsedWidth);
    QVBoxLayout *sidebarLayout = new QVBoxLayout(sidebar);
    sidebarLayout->setContentsMargins(0, 8, 0, 8);
    sidebarLayout->setSpacing(2);

    monitorNavBtn = new QPushButton(QStringLiteral("  ■  Monitor"), sidebar);
    monitorNavBtn->setProperty("cssClass", "navBtn");
    monitorNavBtn->setCursor(Qt::PointingHandCursor);
    monitorNavBtn->setToolTip(QStringLiteral("Monitor"));
    connect(monitorNavBtn, &QPushButton::clicked, this, [this]() { switchPage(0); });
    sidebarLayout->addWidget(monitorNavBtn);

    simulatorNavBtn = new QPushButton(QStringLiteral("  ⚙  Simulator"), sidebar);
    simulatorNavBtn->setProperty("cssClass", "navBtn");
    simulatorNavBtn->setCursor(Qt::PointingHandCursor);
    simulatorNavBtn->setToolTip(QStringLiteral("Simulator"));
    connect(simulatorNavBtn, &QPushButton::clicked, this, [this]() { switchPage(1); });
    sidebarLayout->addWidget(simulatorNavBtn);

    sidebarLayout->addStretch();
    bodyLayout->addWidget(sidebar);

    // Separator line
    QFrame *sep = new QFrame(root);
    sep->setFrameShape(QFrame::VLine);
    sep->setStyleSheet("color: #30363d;");
    bodyLayout->addWidget(sep);

    // Content stack
    stack = new QStackedWidget(root);
    monitorPage = new MonitorPage(stack);
    simulatorPage = new SimulatorPage(stack);

    // Forward simulator running state to title bar
    connect(simulatorPage, &SimulatorPage::simulatorRunningChanged, this, &MainWindow::setSimulatorRunning);

    stack->addWidget(monitorPage);
    stack->addWidget(simulatorPage);
    bodyLayout->addWidget(stack, 1);

    rootLayout->addLayout(bodyLayout, 1);

    // === Status Bar ===
    QWidget *statusBarWidget = new QWidget(root);
    statusBarWidget->setProperty("cssClass", "statusBar");
    statusBarWidget->setFixedHeight(28);
    QHBoxLayout *statusLayout = new QHBoxLayout(statusBarWidget);
    statusLayout->setContentsMargins(16, 2, 16, 2);

    statusLabel = new QLabel(QStringLiteral("Ready."), statusBarWidget);
    statusLabel->setStyleSheet("color: #8899aa; background: transparent; font-size: 11px;");
    statusLayout->addWidget(statusLabel);

    rootLayout->addWidget(statusBarWidget);
}

void MainWindow::switchPage(int index)
{
    stack->setCurrentIndex(index);
    monitorNavBtn->setProperty("selected", index == 0);
    simulatorNavBtn->setProperty("selected", index == 1);
    // Force style refresh
    monitorNavBtn->style()->unpolish(monitorNavBtn);
    monitorNavBtn->style()->polish(monitorNavBtn);
    simulatorNavBtn->style()->unpolish(simulatorNavBtn);
    simulatorNavBtn->style()->polish(simulatorNavBtn);
}

void MainWindow::setSimulatorRunning(bool running)
{
    if (running) {
        simStatusLabel->setText(QStringLiteral("● Sim Running"));
        simStatusLabel->setStyleSheet("color: #22c55e; background: transparent; font-size: 11px;");
    } else {
        simStatusLabel->setText(QStringLiteral("● Sim Idle"));
        simStatusLabel->setStyleSheet("color: #8b949e; background: transparent; font-size: 11px;");
    }
}

void MainWindow::setStatusMessage(const QString &message)
{
    statusLabel->setText(message);
}

} // namespace DncScada
