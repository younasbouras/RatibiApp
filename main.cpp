#include <QGuiApplication>
#include <QQmlApplicationEngine>
#include <QQmlContext>
#include <QObject>
#include <QAbstractListModel>
#include <QVector>
#include <QString>
#include <QDateTime>
#include <QJsonObject>
#include <QJsonArray>
#include <QJsonDocument>
#include <QFile>
#include <QDebug>

// ============================================================================
// 1. نماذج البيانات (Data Models & Core Logic)
// ============================================================================

struct Transaction {
    QString id;
    QString type; // "INCOME" or "EXPENSE"
    double amount;
    QString category; // المصدر أو السبب
    QString date;
    QString walletId;
};

struct Loan {
    QString personName;
    double amount; // موجب: مستلمة (دَيْن عليّ)، سالب: مسلمة (دَيْن لي)
    QString date;
};

struct Asset {
    QString name;
    double purchasePrice;
    double accumulatedDepreciation; // مبلغ الإهتلاك
    double currentValue; // السعر الحالي للتقييم
};

struct Wallet {
    QString name;
    double balance;
};

// المحرك الرئيسي للتطبيق المربوط مع الواجهة
class RatibiEngine : public QObject {
    Q_OBJECT
    Q_PROPERTY(double totalIncome READ totalIncome NOTIFY dataChanged)
    Q_PROPERTY(double totalExpenses READ totalExpenses NOTIFY dataChanged)
    Q_PROPERTY(double remainingBalance READ remainingBalance NOTIFY dataChanged)
    Q_PROPERTY(QString currentLanguage READ currentLanguage WRITE setLanguage NOTIFY languageChanged)
    Q_PROPERTY(QString themeColor READ themeColor WRITE setThemeColor NOTIFY themeChanged)
    Q_PROPERTY(bool isDarkMode READ isDarkMode WRITE setDarkMode NOTIFY themeChanged)

public:
    explicit RatibiEngine(QObject *parent = nullptr) : QObject(parent), m_language("ar"), m_themeColor("#2196F3"), m_isDarkMode(false) {
        // إضافة محفظة إفتراضية شاملة
        m_wallets.append({"المحفظة الرئيسية (الإجمالي)", 0.0});
    }

    // --- حساب الإحصائيات ---
    double totalIncome() const {
        double sum = 0;
        for (const auto &t : m_transactions) if (t.type == "INCOME") sum += t.amount;
        return sum;
    }

    double totalExpenses() const {
        double sum = 0;
        for (const auto &t : m_transactions) if (t.type == "EXPENSE") sum += t.amount;
        return sum;
    }

    double remainingBalance() const {
        return totalIncome() - totalExpenses();
    }

    // --- إدارة المعاملات (المصاريف والمداخيل) ---
    Q_INVOKABLE void addTransaction(const QString &type, double amount, const QString &category, const QString &walletName) {
        QString date = QDateTime::currentDateTime().toString("yyyy-MM-dd hh:mm");
        m_transactions.append({QString::number(m_transactions.size() + 1), type, amount, category, date, walletName});
        
        // تحديث المحفظة
        for (auto &w : m_wallets) {
            if (w.name == walletName || w.name.contains("الإجمالي")) {
                if (type == "INCOME") w.balance += amount;
                else w.balance -= amount;
            }
        }
        emit dataChanged();
    }

    // --- ميزة القروض ---
    Q_INVOKABLE void updateLoan(const QString &person, double deltaAmount) {
        bool found = false;
        for (auto &l : m_loans) {
            if (l.personName == person) {
                l.amount += deltaAmount;
                found = true;
                break;
            }
        }
        if (!found) {
            m_loans.append({person, deltaAmount, QDateTime::currentDateTime().toString("yyyy-MM-dd")});
        }
        emit dataChanged();
    }

    // --- جدول الإهتلاك والممتلكات ---
    Q_INVOKABLE QVariantMap calculateAsset(double purchasePrice, double depreciation, double currentMarketPrice) {
        double priceAfterDepreciation = purchasePrice - depreciation;
        double gainOrLoss = currentMarketPrice - priceAfterDepreciation;
        
        QVariantMap result;
        result["priceAfterDepreciation"] = priceAfterDepreciation;
        result["gainOrLoss"] = gainOrLoss; // قيمة موجبة = فائض، سالبة = خسارة قيمة
        return result;
    }

    // --- المبيعات ---
    Q_INVOKABLE void recordSale(const QString &itemName, double salePrice) {
        addTransaction("INCOME", salePrice, "بيع: " + itemName, "المحفظة الرئيسية (الإجمالي)");
    }

    // --- سجل الآلة الحاسبة (20 عملية) ---
    Q_INVOKABLE void addCalcHistory(const QString &expression, const QString &result) {
        if (m_calcHistory.size() >= 20) m_calcHistory.removeFirst();
        m_calcHistory.append(expression + " = " + result);
        emit calcHistoryChanged();
    }

    Q_INVOKABLE QStringList getCalcHistory() const { return m_calcHistory; }

    // --- الإعدادات والمظهر ---
    QString currentLanguage() const { return m_language; }
    void setLanguage(const QString &lang) { m_language = lang; emit languageChanged(); }

    QString themeColor() const { return m_themeColor; }
    void setThemeColor(const QString &color) { m_themeColor = color; emit themeChanged(); }

    bool isDarkMode() const { return m_isDarkMode; }
    void setDarkMode(bool dark) { m_isDarkMode = dark; emit themeChanged(); }

    // --- البصمة والبيومترية (محاكاة الربط مع النظام) ---
    Q_INVOKABLE bool authenticateBiometric() {
        // يتم الربط مع Android BiometricPrompt أو iOS LocalAuthentication عبر JNI/Objective-C
        return true; 
    }

signals:
    void dataChanged();
    void languageChanged();
    void themeChanged();
    void calcHistoryChanged();

private:
    QVector<Transaction> m_transactions;
    QVector<Loan> m_loans;
    QVector<Asset> m_assets;
    QVector<Wallet> m_wallets;
    QStringList m_calcHistory;
    
    QString m_language;
    QString m_themeColor;
    bool m_isDarkMode;
};

// ============================================================================
// 2. واجهة المستخدم والتصميم العصري الإنسيابي (QML Engine)
// ============================================================================

static const char UI_QML[] = R"(
import QtQuick 2.15
import QtQuick.Controls 2.15
import QtQuick.Layouts 1.15

ApplicationWindow {
    id: window
    width: 390
    height: 844
    visible: true
    title: qsTr("راتبي - Ratibi")
    
    // الألوان الديناميكية
    property color primaryColor: engine.themeColor
    property color bgColor: engine.isDarkMode ? "#121212" : "#F5F7FA"
    property color cardBg: engine.isDarkMode ? "#1E1E1E" : "#FFFFFF"
    property color textColor: engine.isDarkMode ? "#FFFFFF" : "#212121"

    color: bgColor

    // --- الشريط العلوي (Header) ---
    header: ToolBar {
        background: Rectangle { color: primaryColor }
        RowLayout {
            anchors.fill: parent
            anchors.leftMargin: 16
            anchors.rightMargin: 16
            Label {
                text: "راتبي 💰"
                font.bold: true
                font.pixelSize: 20
                color: "white"
            }
            Item { Layout.fillWidth: true }
            Button {
                text: engine.isDarkMode ? "☀️" : "🌙"
                flat: true
                onClicked: engine.isDarkMode = !engine.isDarkMode
            }
        }
    }

    // --- التنقل بالإيماءات والسحب (SwipeView) ---
    SwipeView {
        id: swipeView
        anchors.fill: parent
        currentIndex: tabBar.currentIndex

        // 1. الرئيسية والملخص Daily & Summary
        Page {
            background: Rectangle { color: bgColor }
            ColumnLayout {
                anchors.fill: parent
                anchors.margins: 16
                spacing: 15

                // بطاقة الملخص المالي
                Rectangle {
                    Layout.fillWidth: true
                    height: 140
                    radius: 20
                    color: primaryColor
                    
                    ColumnLayout {
                        anchors.centerIn: parent
                        Label { text: "الباقي الإجمالي"; color: "#E0E0E0"; font.pixelSize: 14 }
                        Label { text: engine.remainingBalance.toFixed(2) + " د.ج"; color: "white"; font.pixelSize: 28; font.bold: true }
                        RowLayout {
                            spacing: 20
                            Label { text: "▲ دخل: " + engine.totalIncome.toFixed(2); color: "#81C784" }
                            Label { text: "▼ مصاريف: " + engine.totalExpenses.toFixed(2); color: "#E57373" }
                        }
                    }
                }

                // قسم إضافة معاملة سريعة (مصاريف اليومي/الكبيرة/المداخيل)
                Label { text: "إضافة معاملة جديدة"; font.bold: true; color: textColor }
                
                RowLayout {
                    Layout.fillWidth: true
                    TextField { id: amountInput; placeholderText: "المبلغ"; Layout.fillWidth: true; inputMethodHints: Qt.ImhFormattedNumbersOnly }
                    TextField { id: categoryInput; placeholderText: "السبب / المصدر"; Layout.fillWidth: true }
                }

                RowLayout {
                    Layout.fillWidth: true
                    Button {
                        text: "+ دخل"
                        Layout.fillWidth: true
                        highlighted: true
                        onClicked: {
                            if(amountInput.text !== "") {
                                engine.addTransaction("INCOME", parseFloat(amountInput.text), categoryInput.text, "المحفظة الرئيسية");
                                amountInput.clear(); categoryInput.clear();
                            }
                        }
                    }
                    Button {
                        text: "- مصاريف"
                        Layout.fillWidth: true
                        onClicked: {
                            if(amountInput.text !== "") {
                                engine.addTransaction("EXPENSE", parseFloat(amountInput.text), categoryInput.text, "المحفظة الرئيسية");
                                amountInput.clear(); categoryInput.clear();
                            }
                        }
                    }
                }
                Item { Layout.fillHeight: true }
            }
        }

        // 2. الممتلكات والإهتلاك (Assets & Depreciation)
        Page {
            background: Rectangle { color: bgColor }
            ColumnLayout {
                anchors.fill: parent
                anchors.margins: 16
                Label { text: "حساب إهتلاك الممتلكات"; font.bold: true; font.pixelSize: 18; color: textColor }
                
                TextField { id: buyPrice; placeholderText: "سعر الشراء"; Layout.fillWidth: true }
                TextField { id: depAmount; placeholderText: "مبلغ الإهتلاك التراكمي"; Layout.fillWidth: true }
                TextField { id: currentPrice; placeholderText: "السعر الحالي في السوق"; Layout.fillWidth: true }

                Button {
                    text: "احسب الإهتلاك والقيمة"
                    Layout.fillWidth: true
                    onClicked: {
                        var res = engine.calculateAsset(parseFloat(buyPrice.text), parseFloat(depAmount.text), parseFloat(currentPrice.text));
                        resLabel.text = "السعر بعد الإهتلاك: " + res.priceAfterDepreciation + "\nالفائض/الخسارة: " + res.gainOrLoss;
                    }
                }

                Label { id: resLabel; font.pixelSize: 16; color: primaryColor; font.bold: true }
                Item { Layout.fillHeight: true }
            }
        }

        // 3. الآلة الحاسبة مع السجل
        Page {
            background: Rectangle { color: bgColor }
            ColumnLayout {
                anchors.fill: parent
                anchors.margins: 16
                Label { text: "آلة حاسبة سريعة (سجل آخر 20 عملية)"; font.bold: true; color: textColor }
                
                TextField { id: calcInput; placeholderText: "مثال: 1500 + 350"; Layout.fillWidth: true }
                Button {
                    text: "حساب وتخزين"
                    Layout.fillWidth: true
                    onClicked: {
                        try {
                            var val = eval(calcInput.text);
                            engine.addCalcHistory(calcInput.text, val.toString());
                            calcInput.text = val.toString();
                        } catch(e) {
                            calcInput.text = "خطأ";
                        }
                    }
                }

                ListView {
                    Layout.fillWidth: true
                    Layout.fillHeight: true
                    model: engine.getCalcHistory()
                    delegate: ItemDelegate {
                        text: modelData
                        width: parent.width
                        onClicked: calcInput.text = modelData.split('=')[0].trim()
                    }
                }
            }
        }

        // 4. الإعدادات والخصائص (الثيمات، البصمة، اللغة)
        Page {
            background: Rectangle { color: bgColor }
            ColumnLayout {
                anchors.fill: parent
                anchors.margins: 16
                spacing: 12
                
                Label { text: "تخصيص المظهر واللغة"; font.bold: true; font.pixelSize: 18; color: textColor }
                
                Label { text: "اختر اللون الرئيسي:"; color: textColor }
                RowLayout {
                    spacing: 8
                    Repeater {
                        model: ["#2196F3", "#4CAF50", "#F44336", "#E91E63", "#000000", "#FF9800", "#FFEB3B", "#795548", "#9C27B0"]
                        Rectangle {
                            width: 32; height: 32; radius: 16; color: modelData
                            MouseArea { anchors.fill: parent; onClicked: engine.themeColor = modelData }
                        }
                    }
                }

                Button {
                    text: "تفعيل الدخول ببصمة الوجه / الأصبع 🔒"
                    Layout.fillWidth: true
                    onClicked: {
                        if(engine.authenticateBiometric()) {
                            authStatus.text = "تم التوثيق بنجاح!";
                        }
                    }
                }
                Label { id: authStatus; color: "green" }

                Item { Layout.fillHeight: true }
            }
        }
    }

    // --- شريط التنقل السفي العصري (Bottom TabBar) ---
    footer: TabBar {
        id: tabBar
        currentIndex: swipeView.currentIndex
        background: Rectangle { color: cardBg }

        TabButton { text: "الرئيسية" }
        TabButton { text: "الممتلكات" }
        TabButton { text: "الحاسبة" }
        TabButton { text: "الإعدادات" }
    }
}
)";

// ============================================================================
// 3. نقطة الانطلاق لتطبيق الهواتف (Main Function)
// ============================================================================

int main(int argc, char *argv[])
{
    // تحسين الدقة والشاشات عالية الكثافة للهواتف الذكية (High DPI)
    QCoreApplication::setAttribute(Qt::AA_EnableHighDpiScaling);
    QGuiApplication app(argc, argv);

    QQmlApplicationEngine qmlEngine;
    RatibiEngine ratibiCore;

    // ربط محرك C++ بـ QML
    qmlEngine.rootContext()->setContextProperty("engine", &ratibiCore);

    // تحميل واجهة المستخدم المدمجة
    qmlEngine.loadData(QByteArray(UI_QML));

    if (qmlEngine.rootObjects().isEmpty())
        return -1;

    return app.exec();
}
#include "main.moc"
