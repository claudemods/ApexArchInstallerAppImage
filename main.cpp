#include <QApplication>
#include <QMainWindow>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QLabel>
#include <QPushButton>
#include <QProgressBar>
#include <QTextEdit>
#include <QInputDialog>
#include <QMessageBox>
#include <QProcess>
#include <QPalette>
#include <QPixmap>
#include <QStringList>
#include <QTimer>
#include <QFont>
#include <QFileDialog>
#include <QFile>

class InstallerWindow : public QMainWindow {
    Q_OBJECT
public:
    InstallerWindow(QWidget *parent = nullptr) : QMainWindow(parent) {
        // Set up the main window
        setWindowTitle("Apex Arch Installer AppImage");
        resize(800, 600);

        // Set teal and black color scheme
        QPalette palette;
        palette.setColor(QPalette::Window, QColor(0, 128, 128)); // Teal background
        palette.setColor(QPalette::WindowText, Qt::white);       // White text
        palette.setColor(QPalette::Base, QColor(53, 53, 53));    // Dark base for text widget
        palette.setColor(QPalette::Text, Qt::white);             // White text in text widget
        palette.setColor(QPalette::Button, QColor(0, 128, 128)); // Teal buttons
        palette.setColor(QPalette::ButtonText, QColor(255, 215, 0)); // Gold button text
        setPalette(palette);

        // Create header layout
        QWidget *headerWidget = new QWidget(this);
        QVBoxLayout *headerLayout = new QVBoxLayout(headerWidget);
        headerLayout->setAlignment(Qt::AlignCenter); // Center-align everything in the header

        // Add images side by side at the top
        QHBoxLayout *imageLayout = new QHBoxLayout();
        imageLayout->setSpacing(0); // No space between images
        imageLayout->setAlignment(Qt::AlignCenter); // Center-align the images

        // Add ApexBrowser.png image
        QLabel *apexBrowserLabel = new QLabel(this);
        QPixmap apexBrowserPixmap(":/images/ApexBrowser.png"); // Ensure this path is correct
        apexBrowserLabel->setPixmap(apexBrowserPixmap.scaled(64, 64, Qt::KeepAspectRatio));
        imageLayout->addWidget(apexBrowserLabel);

        // Add software.png image beside ApexBrowser.png
        QLabel *softwareLabel = new QLabel(this);
        QPixmap softwarePixmap(":/images/software.png"); // Ensure this path is correct
        softwareLabel->setPixmap(softwarePixmap.scaled(64, 64, Qt::KeepAspectRatio));
        imageLayout->addWidget(softwareLabel);

        // Add the image layout to the header layout
        headerLayout->addLayout(imageLayout);

        // Add title text under the images
        QLabel *titleLabel = new QLabel("Apex Arch Installer AppImage v1.0", this);
        QFont titleFont = titleLabel->font();
        titleFont.setPointSize(16);
        titleLabel->setFont(titleFont);
        titleLabel->setStyleSheet("color: gold;");
        titleLabel->setAlignment(Qt::AlignCenter); // Center-align the text
        headerLayout->addWidget(titleLabel);

        // Add distro.png image (centered and scaled to fit)
        distroLabel = new QLabel(this);
        QPixmap distroPixmap(":/images/distro.png"); // Ensure this path is correct
        if (!distroPixmap.isNull()) {
            // Scale the image to fit within the label while maintaining aspect ratio
            distroLabel->setPixmap(distroPixmap.scaled(
                600, 400, // Maximum width and height
                Qt::KeepAspectRatio, // Maintain aspect ratio
                Qt::SmoothTransformation // Smooth scaling
            ));
        } else {
            QMessageBox::warning(this, "Error", "Failed to load distro image.");
        }
        distroLabel->setAlignment(Qt::AlignCenter);
        headerLayout->addWidget(distroLabel);

        // Main layout
        QVBoxLayout *mainLayout = new QVBoxLayout;

        // Add header to main layout
        mainLayout->addWidget(headerWidget);

        // Command output text box (hidden initially)
        outputText = new QTextEdit(this);
        outputText->setReadOnly(true);
        outputText->setStyleSheet("background-color: gold; color: green; border: 2px solid black;");
        outputText->setVisible(false);
        mainLayout->addWidget(outputText);

        // Progress bar below the command box
        progressBar = new QProgressBar(this);
        progressBar->setRange(0, 100);
        progressBar->setValue(0); // Start at 0%
        progressBar->setStyleSheet(
            "QProgressBar { border: 2px solid black; background: white; }" // Changed background to white
            "QProgressBar::chunk { background: gold; }");
        progressBar->setVisible(false); // Initially hidden
        mainLayout->addWidget(progressBar);

        // Log button and back button
        QHBoxLayout *buttonLayout = new QHBoxLayout();
        logButton = new QPushButton("Log", this);
        logButton->setStyleSheet("background-color: teal; color: gold;");
        connect(logButton, &QPushButton::clicked, this, &InstallerWindow::showLog);
        buttonLayout->addStretch(); // Push log button to the right
        buttonLayout->addWidget(logButton);

        backButton = new QPushButton("Back", this);
        backButton->setStyleSheet("background-color: teal; color: gold;");
        backButton->setVisible(false);
        connect(backButton, &QPushButton::clicked, this, &InstallerWindow::showDistroImage);
        buttonLayout->addWidget(backButton);

        mainLayout->addLayout(buttonLayout);

        // Set central widget
        QWidget *centralWidget = new QWidget(this);
        centralWidget->setLayout(mainLayout);
        setCentralWidget(centralWidget);

        // Start the installation process automatically
        QTimer::singleShot(0, this, &InstallerWindow::startInstallation);
    }

private slots:
    void startInstallation() {
        // Ask for sudo password
        bool ok;
        sudoPassword = QInputDialog::getText(this, "Sudo Password", "Enter your sudo password:", QLineEdit::Password, "", &ok);
        if (!ok || sudoPassword.isEmpty()) {
            QMessageBox::warning(this, "Error", "Sudo password is required.");
            return; // Do not quit the application
        }

        // Ask for target drive
        QString drive = QInputDialog::getText(this, "Target Drive", "Enter the target drive (e.g., /dev/sda):", QLineEdit::Normal, "", &ok);
        if (!ok || drive.isEmpty()) {
            QMessageBox::warning(this, "Error", "Target drive is required.");
            return; // Do not quit the application
        }
        this->drive = drive;

        // Ask if the user wants to search default SquashFS locations
        QString searchDefault = QInputDialog::getText(this, "SquashFS Search", "Do you want to search default SquashFS locations? (y/n):");
        QString airootfsPath;
        if (searchDefault.toLower() == "y") {
            // Search default locations
            outputText->append("Checking for airootfs.sfs in default locations...");
            airootfsPath = findSquashFS();
            if (airootfsPath.isEmpty()) {
                QMessageBox::warning(this, "Error", "SquashFS file not found in default locations.");
                return; // Do not quit the application
            }
            outputText->append("Found SquashFS file at: " + airootfsPath);
        } else {
            // Ask for custom SquashFS location
            airootfsPath = QFileDialog::getOpenFileName(this, "Select SquashFS File", "/", "SquashFS Files (*.sfs)");
            if (airootfsPath.isEmpty()) {
                QMessageBox::warning(this, "Error", "SquashFS file is required.");
                return; // Do not quit the application
            }
        }

        // Verify SquashFS file exists using QProcess
        QProcess testProcess;
        testProcess.start("sudo", QStringList() << "-S" << "test" << "-f" << airootfsPath);
        testProcess.write((sudoPassword + "\n").toUtf8());
        testProcess.closeWriteChannel();
        if (!testProcess.waitForFinished() || testProcess.exitCode() != 0) {
            QMessageBox::warning(this, "Error", "Invalid SquashFS file path.");
            return; // Do not quit the application
        }

        // Start the chained execution of commands
        currentStep = 0;
        this->airootfsPath = airootfsPath;
        executeNextCommand();
    }

    void executeNextCommand() {
        if (currentStep >= commandList.size()) {
            // Installation completed, show post-installation menu
            displayPostInstallMenu();
            return;
        }

        const auto &command = commandList[currentStep];
        QString program = command.program;
        QStringList arguments = command.arguments;

        // Replace placeholders with actual values
        for (int i = 0; i < arguments.size(); ++i) {
            arguments[i].replace("{drive}", drive);
            arguments[i].replace("{airootfsPath}", airootfsPath);
        }

        QProcess *process = new QProcess(this);
        connect(process, &QProcess::finished, this, &InstallerWindow::onCommandFinished);

        outputText->append("Executing: " + program + " " + arguments.join(" "));
        process->start(program, arguments);
        process->write((sudoPassword + "\n").toUtf8());
        process->closeWriteChannel();
    }

    void onCommandFinished(int exitCode, QProcess::ExitStatus exitStatus) {
        QProcess *process = qobject_cast<QProcess *>(sender());
        if (!process) {
            return;
        }

        if (exitStatus != QProcess::NormalExit || exitCode != 0) {
            outputText->append("Error: Command failed with exit code " + QString::number(exitCode));
            outputText->append(process->readAllStandardError());
            QMessageBox::critical(this, "Error", "Installation failed. Check the log for details.");
            process->deleteLater();
            return;
        }

        outputText->append("Command completed successfully.");
        process->deleteLater();

        // Update progress bar based on the current step only if the command was successful
        if (exitStatus == QProcess::NormalExit && exitCode == 0) {
            progressBar->setVisible(true); // Show the progress bar when updating
            if (currentStep == 7) { // mount /mnt
                progressBar->setValue(25);
            } else if (currentStep == 9) { // unsquashfs
                progressBar->setValue(50);
            } else if (currentStep == 11) { // genfstab
                progressBar->setValue(75);
            } else if (currentStep == 12) { // arch-chroot
                progressBar->setValue(80);
            } else if (currentStep == 13) { // umount commands
                progressBar->setValue(100);
            }
        }

        // Move to the next command
        currentStep++;
        executeNextCommand();
    }

    QString findSquashFS() {
        // Default SquashFS locations
        QStringList defaultLocations = {
            "/run/archiso/copytoram/arch/x86_64/airootfs.sfs",
            "/run/archiso/bootmnt/arch/x86_64/airootfs.sfs",
            "/run/archiso/copytoram/airootfs.sfs"
        };
        for (const QString &location : defaultLocations) {
            QProcess testProcess;
            testProcess.start("sudo", QStringList() << "-S" << "test" << "-f" << location);
            testProcess.write((sudoPassword + "\n").toUtf8());
            testProcess.closeWriteChannel();
            if (testProcess.waitForFinished() && testProcess.exitCode() == 0) {
                return location;
            }
        }
        return "";
    }

    void displayPostInstallMenu() {
        QString choice = QInputDialog::getText(this, "Post Install Extras",
                                               "=== Post Install Extras Option ===\n"
                                               "1. Chroot into the new system\n"
                                               "2. Reboot the system\n"
                                               "3. Exit\n"
                                               "Enter your choice (1/2/3):");

        if (choice == "1") {
            // Chroot into the new system
            outputText->append("Chrooting into the new system...");
            QProcess chrootProcess;
            chrootProcess.start("sudo", QStringList() << "-S" << "arch-chroot" << "/mnt" << "/bin/bash");
            chrootProcess.write((sudoPassword + "\n").toUtf8());
            chrootProcess.closeWriteChannel();
            if (!chrootProcess.waitForFinished()) {
                QMessageBox::warning(this, "Error", "Failed to chroot into the new system.");
            }
        } else if (choice == "2") {
            // Reboot the system
            outputText->append("Rebooting the system...");
            QProcess rebootProcess;
            rebootProcess.start("sudo", QStringList() << "-S" << "reboot");
            rebootProcess.write((sudoPassword + "\n").toUtf8());
            rebootProcess.closeWriteChannel();
            if (!rebootProcess.waitForFinished()) {
                QMessageBox::warning(this, "Error", "Failed to reboot the system.");
            }
        } else if (choice == "3") {
            // Exit the application
            outputText->append("Exiting...");
            QApplication::quit();
        } else {
            QMessageBox::warning(this, "Error", "Invalid choice. Exiting.");
            QApplication::quit();
        }
    }

    void showLog() {
        distroLabel->hide(); // Hide the image without altering its properties
        outputText->show();
        logButton->hide();
        backButton->show();
    }

    void showDistroImage() {
        distroLabel->show(); // Show the image without reloading it
        outputText->hide();
        logButton->show();
        backButton->hide();
    }

private:
    struct Command {
        QString program;
        QStringList arguments;
    };

    QTextEdit *outputText;
    QProgressBar *progressBar;
    QLabel *distroLabel;
    QPushButton *logButton;
    QPushButton *backButton;
    QString sudoPassword;
    QString drive;
    QString airootfsPath;
    int currentStep = 0;

    QList<Command> commandList = {
        {"sudo", {"wipefs", "--all", "{drive}"}},
        {"sudo", {"parted", "-s", "{drive}", "mklabel", "gpt"}},
        {"sudo", {"parted", "-s", "{drive}", "mkpart", "primary", "fat32", "1MiB", "551MiB"}},
        {"sudo", {"parted", "-s", "{drive}", "set", "1", "esp", "on"}},
        {"sudo", {"parted", "-s", "{drive}", "mkpart", "primary", "ext4", "551MiB", "100%"}},
        {"sudo", {"mkfs.vfat", "{drive}1"}},
        {"sudo", {"mkfs.ext4", "{drive}2"}},
        {"sudo", {"mount", "{drive}2", "/mnt"}},
        {"sudo", {"mkdir", "-p", "/mnt/boot/efi"}},
        {"sudo", {"mount", "{drive}1", "/mnt/boot/efi"}},
        {"sudo", {"unsquashfs", "-f", "-d", "/mnt", "{airootfsPath}"}},
        {"sudo", {"genfstab", "-U", "-p", "/mnt", ">>", "/mnt/etc/fstab"}},
        {"sudo", {"arch-chroot", "/mnt", "/bin/bash", "-c", "grub-install --target=x86_64-efi --efi-directory=/boot/efi --bootloader-id=GRUB --recheck && grub-mkconfig -o /boot/grub/grub.cfg && mkinitcpio -P"}},
        {"sudo", {"umount", "-l", "/mnt/boot/efi"}}, // Lazy unmount
        {"sudo", {"umount", "-l", "/mnt"}}           // Lazy unmount
    };
};

int main(int argc, char *argv[]) {
    QApplication app(argc, argv);
    InstallerWindow window;
    window.show();
    return app.exec();
}

#include "main.moc"
