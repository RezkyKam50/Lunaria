#include <QApplication>
#include <QWidget>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QPushButton>
#include <QLabel>
#include <QLineEdit>
#include <QTextEdit>
#include <QMessageBox>
#include <QSpinBox>
#include <QCheckBox>

class MainWindow : public QWidget
{
    Q_OBJECT   

public:
    MainWindow(QWidget *parent = nullptr) : QWidget(parent)
    {
 
        setWindowTitle("Simple Qt6 Application");
        setFixedSize(400, 300);   
 
        auto *mainLayout = new QVBoxLayout(this);
 
        auto *titleLabel = new QLabel("Welcome to Qt6 Application!");
        titleLabel->setAlignment(Qt::AlignCenter);
        mainLayout->addWidget(titleLabel);
 
        auto *inputLayout = new QHBoxLayout();
        
        auto *nameLabel = new QLabel("Name:");
        nameInput = new QLineEdit();
        nameInput->setPlaceholderText("Enter your name");
        
        inputLayout->addWidget(nameLabel);
        inputLayout->addWidget(nameInput);
        mainLayout->addLayout(inputLayout);
 
        auto *counterLayout = new QHBoxLayout();
        
        auto *counterLabel = new QLabel("Counter:");
        counterSpinBox = new QSpinBox();
        counterSpinBox->setRange(0, 100);  
        counterSpinBox->setValue(0);
        
        counterLayout->addWidget(counterLabel);
        counterLayout->addWidget(counterSpinBox);
        mainLayout->addLayout(counterLayout);
 
        auto *checkBox = new QCheckBox("Enable features");
        checkBox->setChecked(true);
        mainLayout->addWidget(checkBox);
 
        auto *buttonLayout = new QHBoxLayout();
         
        auto *greetButton = new QPushButton("Greet");
        auto *clearButton = new QPushButton("Clear");
        auto *quitButton = new QPushButton("Quit");
        
        buttonLayout->addWidget(greetButton);
        buttonLayout->addWidget(clearButton);
        buttonLayout->addWidget(quitButton);
        mainLayout->addLayout(buttonLayout);
 
        textDisplay = new QTextEdit();
        textDisplay->setReadOnly(true);   
        textDisplay->setMaximumHeight(100);  
        mainLayout->addWidget(textDisplay);
 
        connect(greetButton, &QPushButton::clicked, this, &MainWindow::onGreetButtonClicked);
        connect(clearButton, &QPushButton::clicked, this, &MainWindow::onClearButtonClicked);
        connect(quitButton, &QPushButton::clicked, this, &MainWindow::onQuitButtonClicked);
         
        connect(checkBox, &QCheckBox::stateChanged, this, &MainWindow::onCheckBoxStateChanged);
         
        connect(counterSpinBox, QOverload<int>::of(&QSpinBox::valueChanged),
                this, &MainWindow::onCounterValueChanged);

        textDisplay->append("Application started. Ready to use!");
    }

private slots:
    void onGreetButtonClicked()
    {
        QString name = nameInput->text().trimmed();
        
        if (name.isEmpty()) {
            QMessageBox::warning(this, "Input Error", "Please enter your name!");
            return;
        }
         
        QString message = QString("Hello, %1! Welcome to Qt6!").arg(name);
        textDisplay->append(message);
 
        QMessageBox::information(this, "Greeting", message);
    }

    void onClearButtonClicked()
    {
        textDisplay->clear();
        textDisplay->append("Text cleared!");
    }
    void onQuitButtonClicked()
    {
        auto reply = QMessageBox::question(this, "Confirm Quit", 
                                          "Are you sure you want to quit?",
                                          QMessageBox::Yes | QMessageBox::No);
        
        if (reply == QMessageBox::Yes) {
            QApplication::quit();
        }
    }
    void onCheckBoxStateChanged(int state)
    {
        bool enabled = (state == Qt::Checked);
        QString status = enabled ? "enabled" : "disabled";
        textDisplay->append(QString("Features %1").arg(status));
    }
    void onCounterValueChanged(int value)
    {
        textDisplay->append(QString("Counter value changed to: %1").arg(value));
    }

private:
    QLineEdit *nameInput;
    QTextEdit *textDisplay;
    QSpinBox *counterSpinBox;
};

int main(int argc, char *argv[])
{
    QApplication app(argc, argv);
    
    app.setApplicationName("Simple Qt6 App");
    app.setApplicationVersion("1.0");

    MainWindow window;
    window.show();   

    return app.exec();
}

#include "lunaria.moc"