#include "mainwindow.h"
#include "ui_mainwindow.h"
#include <QIcon>
#include <QDebug>
#include <QMessageBox>
#include <QFileInfo>
#include <QFileDialog>
#include <QTextStream>
#include <QTableView>
#include <QVector>

//# of ms to display messages in status bar for.
#define MESSAGE_DISPLAY_LENGTH 4000

MainWindow::MainWindow(QWidget *parent) :
    QMainWindow(parent),
    ui(new Ui::MainWindow)
{
    logger = new Logger();
    ui->setupUi(this);
    //setup window attributes
    setWindowIcon(QIcon(":/imgs/money_management.gif"));
    setWindowTitle("Money Management");
    setFixedSize(width(), height());
    ui->dateEdit->setDate(QDate::currentDate());
    //add different modes to combo box
    ui->comboBoxMode->addItem("Deposit");
    ui->comboBoxMode->addItem("Withdraw");
    //initially have nothing selected
    ui->comboBoxMode->setCurrentIndex(-1);
    //only allow valid doubles for the deposit/withdrawal amt.
    ui->lineEditDepWithdr->setValidator(new QDoubleValidator(1, 10000000, 2, ui->lineEditDepWithdr));
    //the directory where the database lives.
    db_path = QDir::currentPath() + "/transaction_db.db";
    logger->log(Logger::DEBUG, "DB Path: " + db_path);    
    //init objects.
    edit_trans_model = NULL;
    edit_trans_view = NULL;
    view_all_transactions_model = NULL;
    view_all_transactions_view = NULL;
    //sets up the database
    setup_database();
    
    //query database to get last transaction's balance and set the total label.
    double last_balance = get_last_transaction_balance();
    ui->labelTotal->setText("Total: " + format.toCurrencyString(last_balance));
    logger->log(Logger::DEBUG, "Setting initial total to: " + format.toCurrencyString(last_balance));
}

double MainWindow::get_last_transaction_balance(){
    //queries the database for the last known balance and sets the total label accordingly.
    open_database();
    double result = 0;
    QSqlQuery qry = transaction_db.exec("SELECT balance FROM transactions ORDER BY id DESC LIMIT 1;");
    logger->log(Logger::DEBUG, "Get last transaction balance qry", qry.lastError().text());
    if(qry.next()){
        result = qry.value(0).toDouble();
    }else{
        result = 0;
    }
    close_database();
    return result;
}

//free memory
MainWindow::~MainWindow()
{
    logger->log(Logger::DEBUG, "freeing memory");
    delete logger;
    delete ui;
}

/*
 * Sets up the sqlite database
 * creates db file if doesn't exist, and creates the necessary table(s)
 * 
*/
void MainWindow::setup_database(){
    logger->log(Logger::DEBUG, "Setting up database");
    transaction_db = QSqlDatabase::addDatabase("QSQLITE");
    transaction_db.setDatabaseName(db_path);
    QFileInfo db_info(db_path);
    bool exists = db_info.exists();
    logger->log(Logger::DEBUG, "Database exists: " + (exists ? QString("True") : QString("False")));    
    bool status = transaction_db.open();
    logger->log(Logger::DEBUG, "Database open status: " + (status ? QString("True") : QString("False")));        
    if(!status){
        logger->log(Logger::CRITICAL, "Failed to open database");            
        ui->statusBar->showMessage("Error connecting to database", MESSAGE_DISPLAY_LENGTH);
        QMessageBox::critical(this, "Error Connecting To Database", "Error opening database, please restart the program");
        ui->lineEditDepWithdr->setEnabled(false);
        ui->lineEditDescription->setEnabled(false);
        ui->pushButtonSubmit->setEnabled(false);
        ui->comboBoxMode->setEnabled(false);
        ui->dateEdit->setEnabled(false);
        ui->menuBar->setEnabled(false);
        return;
    }
    logger->log(Logger::DEBUG, "Succcessfully connected");
    ui->statusBar->showMessage("Connected...", MESSAGE_DISPLAY_LENGTH);
    if(!exists){
        logger->log(Logger::DEBUG, "Creating transaction table");
        QSqlQuery create_transaction_table_qry = transaction_db.exec("CREATE TABLE transactions(id INTEGER PRIMARY KEY AUTOINCREMENT, description TEXT, mode TEXT, trans_amount DOUBLE, balance DOUBLE, date_added DATE);");
        logger->log(Logger::DEBUG, "Create trans table qry" ,create_transaction_table_qry.lastError().text());
    }
    close_database();
}

/*
 * Opens the database for reading/writing.
*/
bool MainWindow::open_database(){
    bool status = transaction_db.open();
    if(status){
        logger->log(Logger::DEBUG, "Connected to database");
        ui->statusBar->showMessage("Connected...", MESSAGE_DISPLAY_LENGTH);
    }else{
        logger->log(Logger::CRITICAL, "Error opening database");
        ui->statusBar->showMessage("Error connecting to database", MESSAGE_DISPLAY_LENGTH);
        QMessageBox::critical(this, "Error Connecting To Database", "Error opening database, please restart the program");
        ui->lineEditDepWithdr->setEnabled(false);
        ui->lineEditDescription->setEnabled(false);
        ui->pushButtonSubmit->setEnabled(false);
        ui->comboBoxMode->setEnabled(false);
        ui->dateEdit->setEnabled(false);
        ui->menuBar->setEnabled(false);
    }
    return status;
}

/*
 * Closes the database
*/
void MainWindow::close_database(){
    //  bool status = transaction_db.commit();
    logger->log(Logger::DEBUG, "Closing database");
    transaction_db.close();
}


void MainWindow::on_actionAbout_triggered()
{
}

/*
 * Triggered when the user is adding a new transaction
 * Checks to make sure an amount, mode and description were provided, otherwise throws error
 * Checks to make sure a withdrawal is not more than the current balance
 * Adds the transaction to the database if all is well
 * 
*/
void MainWindow::on_pushButtonSubmit_clicked()
{
    if(!open_database()){
        QMessageBox::critical(this, "Error Saving Transaction", "Error saving the transaction, please try again");
        logger->log(Logger::CRITICAL, "Error opening database to save new transaction (submit btn). Transaction not saved");
        return;
    }
    QString description = ui->lineEditDescription->text();
    QString mode = ui->comboBoxMode->currentText();
    double amount = ui->lineEditDepWithdr->text().toDouble();
    
    if(amount == 0 || description == "" || mode == ""){
        QMessageBox::critical(this, "Invalid Input", "Please provide a description, amount and if this transaction is a deposit or withdrawal.");
        logger->log(Logger::DEBUG, "Invalid Input\nAmount: " + QString::number(amount) + " Description: " + description + " Mode: " + mode);
        return;
    }
    double last_balance = get_last_transaction_balance();
    logger->log(Logger::DEBUG, "Last known balance (submit btn) " + QString::number(last_balance));
    if(mode == "Deposit"){
        logger->log(Logger::DEBUG, "Depositing " + QString::number(amount) + " to " + QString::number(last_balance));
        last_balance += amount;
    }else if(mode == "Withdraw"){
        logger->log(Logger::DEBUG, "Withdrawing " + QString::number(amount) + " from " + QString::number(last_balance));        
        if(last_balance - amount < 0){
            qDebug() << "Can't withdraw " << amount << " from " << last_balance;
            QMessageBox::information(this, "Insufficient Funds", "There is not enough money to withdraw " + format.toCurrencyString(amount));
            return;
        }
        last_balance -= amount;
    }
    QSqlQuery add_transaction_qry;
    add_transaction_qry.prepare("INSERT INTO transactions (description, mode, trans_amount, balance, date_added) VALUES (:desc, :mode, :trans_amount, :balance, :date);");
    add_transaction_qry.bindValue(":desc", description);
    add_transaction_qry.bindValue(":mode", mode);
    add_transaction_qry.bindValue(":trans_amount", amount);
    add_transaction_qry.bindValue(":balance", last_balance);
    add_transaction_qry.bindValue(":date", ui->dateEdit->date());
    bool result = add_transaction_qry.exec();
    logger->log(Logger::DEBUG, "add transaction qry", add_transaction_qry.lastError().text());
    if(result){
        ui->statusBar->showMessage("Transaction saved", MESSAGE_DISPLAY_LENGTH);
        ui->lineEditDepWithdr->setText("");
        ui->lineEditDescription->setText("");
        ui->comboBoxMode->setCurrentIndex(-1);
        ui->pushButtonSubmit->setEnabled(false);
        QTimer::singleShot(1750, this, SLOT(reenable_submit_btn()));
        ui->labelTotal->setText("Total: " + format.toCurrencyString(last_balance));   
        logger->log(Logger::DEBUG, "Transaction saved");
    }else{
        ui->statusBar->showMessage("Error saving transaction", MESSAGE_DISPLAY_LENGTH);
        logger->log(Logger::CRITICAL, "Error saving transaction");
    }
    close_database();
}

/*
 * This function re-enables the submit button after a specified amount of seconds to prevent from spamming the submit button.
 */
void MainWindow::reenable_submit_btn(){
    ui->pushButtonSubmit->setEnabled(true);
}

/*
 * Calls the close function which is handled by the closeEvent function
 * Triggered when the Quit menu item is selected or ctrl + q
 */
void MainWindow::on_actionQuit_triggered()
{
    this->close();
    
}

/*
 * Triggered when the main window is closing
 * Prompts user to quit, if yes checks to see if any other windows are open and deletes them.
*/
void MainWindow::closeEvent(QCloseEvent* event){
    int result = QMessageBox::question(NULL, "Quit?", "Are you sure you want to quit?");
    if(result == QMessageBox::Yes){
        if(edit_trans_view != NULL){
            logger->log(Logger::DEBUG, "Closing edit trans view");
            edit_trans_view->deleteLater();
        }
        if(view_all_transactions_view != NULL){
            logger->log(Logger::DEBUG, "Closing view trans view");            
            view_all_transactions_view->deleteLater();
        }
        logger->log(Logger::DEBUG, "Closing main window & quitting...");        
        event->accept();
    }else{
        event->ignore();
    }
}

/*
 * Exports whatever is currently in the database to a file with a .sql extension
 * Used for backups, etc.
 * User is prompted via a file dialog
*/
void MainWindow::on_actionExport_triggered()
{
    QString filename = QFileDialog::getSaveFileName(this, tr("Export Database"), QDir::currentPath(), tr("Sql File (*.sql)"));
    if(!filename.isEmpty()){
        open_database();
        QSqlQuery transactions_exist_qry = transaction_db.exec("SELECT count(id) FROM transactions;");
        logger->log(Logger::DEBUG, "transactions exist qry", transactions_exist_qry.lastError().text());
        if(transactions_exist_qry.next()){
            if(transactions_exist_qry.value(0) <=0){
                logger->log(Logger::DEBUG, "There are no transactions to export");
                QMessageBox::information(this, "No Transactions Exist", "There are no transactions to export, export cancelled");
                return;
            }
        }else{
            logger->log(Logger::DEBUG, "There are no transactions to export");            
            QMessageBox::information(this, "No Transactions Exist", "There are no transactions to export, export cancelled");
            return;
        }
        logger->log(Logger::DEBUG, "transactions exist, continuing w/ export");        
        QSqlQuery get_all_transactions_qry = transaction_db.exec("SELECT id, description, mode, trans_amount, balance, date_added FROM transactions;");
        logger->log(Logger::DEBUG, "Get all transactions for export qry", get_all_transactions_qry.lastError().text());
        int count = 0;
        QFile export_file(filename);
        if(!export_file.open(QFile::WriteOnly)){
            logger->log(Logger::CRITICAL, "Error opening " + filename + " for export");
            QMessageBox::information(this, "Error Opening File", "Error opening " + filename + " for export, please try again");
            return;
        }
        export_file.write("PRAGMA foreign_keys=OFF;\n");
        export_file.write("BEGIN TRANSACTION;\n");
        export_file.write("CREATE TABLE transactions(id INTEGER PRIMARY KEY AUTOINCREMENT, description TEXT, mode TEXT, trans_amount DOUBLE, balance DOUBLE, date_added DATE);\n");
        QString write_str;
        while(get_all_transactions_qry.next()){
            write_str = "INSERT INTO transactions (id, description, mode, trans_amount, balance, date_added) VALUES (" + get_all_transactions_qry.value(ID).toString() + ", '" + get_all_transactions_qry.value(DESCRIPTION).toString() + "', '" + get_all_transactions_qry.value(MODE).toString() + "', " + get_all_transactions_qry.value(TRANSACTION_AMOUNT).toString() + ", " + get_all_transactions_qry.value(BALANCE).toString() + ", '" + get_all_transactions_qry.value(DATE_ADDED).toString() + "');\n";
            logger->log(Logger::DEBUG, "Writing " + write_str);
            export_file.write(write_str.toUtf8());
            count++;
        }
        export_file.write("COMMIT;");
        QMessageBox::information(this, "Success", QString::number(count) + " transactions exported to " + filename);
        logger->log(Logger::DEBUG, QString(count) + " transactions written to " + filename);
        export_file.flush();
        export_file.close();
        close_database();
    }
}

/*
 * Triggered when user wants to import a database.
 * Drops the current table, overwrites w/ new queries
*/
void MainWindow::on_actionImport_triggered()
{
    QString filename = QFileDialog::getOpenFileName(this, tr("Import Database"), QDir::currentPath(), tr("Sql File (*.sql)"));
    if(!filename.isEmpty()){
        
        //@TODO -- auto backup the database? -- create backups folder...
        int choice = QMessageBox::question(this, "Overwrite existing data?", "This action will overwrite any existing data, are you sure you want to continue?");
        if(choice == QMessageBox::Yes){
            logger->log(Logger::DEBUG, "Overwriting database via import");
            open_database();
            QSqlQuery drop_table_qry = transaction_db.exec("DROP TABLE transactions;");
            logger->log(Logger::DEBUG, "drop table for import qry", drop_table_qry.lastError().text());
            QFile import_file(filename);
            if(!import_file.open(QFile::ReadOnly)){
                logger->log(Logger::CRITICAL, "Error opening " + filename + " for import");
                QMessageBox::information(this, "Error Opening File", "Error opening " + filename + " for import, please try again");
                return;
            }
            QTextStream in(&import_file);
            QStringList errors;
            while(!in.atEnd()){
                QString statement = in.readLine();
                QSqlQuery qry = transaction_db.exec(statement);
                logger->log(Logger::DEBUG, "import qry", qry.lastError().text());
                if(qry.lastError().text() != " "){
                    errors.push_back("Error: " + qry.lastError().text() + "\nStatement: " + statement);
                    logger->log(Logger::CRITICAL, "Error on statement " + statement);
                }
            }
            if(errors.empty()){
                logger->log(Logger::DEBUG, "All data successfully imported");
                QMessageBox::information(this, "Success", "All data successfully imported");
                double last_balance = get_last_transaction_balance();
                ui->labelTotal->setText("Total: " + format.toCurrencyString(last_balance));
            }else{
                logger->log(Logger::DEBUG, QString::number(errors.size()) + " error(s) encountered");
                QMessageBox::warning(this, QString::number(errors.size()) + " Error(s)", "Encountered " + QString::number(errors.size()) + " errors(s)");
            }
            import_file.close();
            close_database();
        }
    }
}

/*
 * Deletes the current table from the database
*/
void MainWindow::on_actionDelete_triggered()
{
    int choice = QMessageBox::question(this, "Are you sure?", "Are you sure you want to delete the entire database? This cannot be undone.");
    if(choice == QMessageBox::Yes){
        open_database();
        QSqlQuery remove_all_records_qry = transaction_db.exec("DELETE FROM transactions;");
        logger->log(Logger::DEBUG, "drop table delete db qry" + remove_all_records_qry.lastError().text());
        close_database();
        if(remove_all_records_qry.lastError().text() == " "){
            logger->log(Logger::DEBUG, "Database sucessfully deleted");
            ui->statusBar->showMessage("Database successfully deleted", MESSAGE_DISPLAY_LENGTH);
        }else{
            logger->log(Logger::DEBUG, "Error deleting database");            
            ui->statusBar->showMessage("Error deleting database", MESSAGE_DISPLAY_LENGTH);
        }
    }
}

/*
 * Displays all of the transactions in the database to the user in Read-Only mode
*/
void MainWindow::on_actionAll_Transactions_triggered()
{
    logger->log(Logger::DEBUG, "Viewing all transactions");
    view_all_transactions_model = new QSqlQueryModel(this);
    open_database();
    view_all_transactions_model->setQuery("SELECT description, mode, trans_amount, balance, date_added FROM transactions;", transaction_db);
    view_all_transactions_model->setHeaderData(0, Qt::Horizontal, tr("Description"));
    view_all_transactions_model->setHeaderData(1, Qt::Horizontal, tr("Mode"));
    view_all_transactions_model->setHeaderData(2, Qt::Horizontal, tr("Transaction Amount"));
    view_all_transactions_model->setHeaderData(3, Qt::Horizontal, tr("Resulting Balance"));
    view_all_transactions_model->setHeaderData(4, Qt::Horizontal, tr("Date Added"));
    
    view_all_transactions_view = new QTableView;
    view_all_transactions_view->setModel(view_all_transactions_model);
    view_all_transactions_view->setWindowIcon(QIcon(":/imgs/money_management.gif"));
    view_all_transactions_view->setWindowTitle("View All Transactions");
    view_all_transactions_view->resizeRowsToContents();
    view_all_transactions_view->resizeColumnsToContents();
    //view->horizontalHeader()->setStretchLastSection(true);
    view_all_transactions_view->setGeometry(this->x(), this->y(), 550, 350);
    view_all_transactions_view->show();
    close_database();
}

/*
 * Triggered when the user wants to edit a transaction(s)
 * The user editing data will send a signal dataChanged which is caught by the slot record_changed, this function handles everything.
 * Displays all transactions from the database, Read/Write
*/
void MainWindow::on_actionTransaction_triggered()
{
    logger->log(Logger::DEBUG, "Editing all transactions");
    edit_trans_model = new QSqlTableModel(this, transaction_db);
    open_database();
    edit_trans_model->setTable("transactions");
    edit_trans_model->setEditStrategy(QSqlTableModel::OnManualSubmit);
    edit_trans_model->select();
    edit_trans_model->setHeaderData(1, Qt::Horizontal, tr("Description"));
    edit_trans_model->setHeaderData(2, Qt::Horizontal, tr("Mode"));
    edit_trans_model->setHeaderData(3, Qt::Horizontal, tr("Transaction Amount"));
    edit_trans_model->setHeaderData(4, Qt::Horizontal, tr("Resulting Balance"));
    edit_trans_model->setHeaderData(5, Qt::Horizontal, tr("Date Added"));
    
    edit_trans_view = new QTableView;
    edit_trans_view->setModel(edit_trans_model);
    edit_trans_view->hideColumn(ID);
    edit_trans_view->hideColumn(BALANCE);
    edit_trans_view->setWindowIcon(QIcon(":/imgs/money_management.gif"));
    edit_trans_view->setWindowTitle("Edit Transactions");
    edit_trans_view->resizeRowsToContents();
    edit_trans_view->resizeColumnsToContents();
    //edit_trans_view->horizontalHeader()->setStretchLastSection(true);
    edit_trans_view->setGeometry(this->x(), this->y(), 400, 350);
    edit_trans_view->show();
    
    connect(edit_trans_model, SIGNAL(dataChanged(QModelIndex,QModelIndex)), this, SLOT(record_changed(QModelIndex,QModelIndex)));
}

/*
 * Handles error checking and updating for the columns that can be edited
 * Quite cumbersome
*/
void MainWindow::record_changed(QModelIndex index_1, QModelIndex index_2){
    Q_UNUSED (index_2);
    //every single time a record is changed, whether by the user or programatically, this function would be called. Since we are using setdata a bunch
    //we tempororaily prevent the connection b/w these. The connection is restored at the end of this function
    disconnect(edit_trans_model, SIGNAL(dataChanged(QModelIndex,QModelIndex)), this, SLOT(record_changed(QModelIndex,QModelIndex)));
    
    /* column 0 = id
   * column 1 = descrip
   * column 2 = mode
   * column 3 = trans amount
   * column 4 = balance
   * column 5 = date added
  */
    QVariant changed_data = edit_trans_model->data(index_1, Qt::DisplayRole);
    int choice = 0;
    switch (index_1.column()) {
    case 1:{
        if(edit_trans_model->submitAll()){
            logger->log(Logger::DEBUG, "description updated");            
            QMessageBox::information(edit_trans_view, "Success", "Description successfully updated");
        }else{
            logger->log(Logger::DEBUG, "error updating description");            
            QMessageBox::warning(edit_trans_view, "Error", "Error updating description, please try again");
        }
        break;
    }
    case 2:{
        if(changed_data.toString() == "Deposit" || changed_data.toString() == "Withdraw"){
            choice = QMessageBox::question(edit_trans_view, "Update Subsequent Transactions?", "All subsequent transactions will have their balances updated to reflect this change, continue?");
            if(choice == QMessageBox::Yes){
                logger->log(Logger::DEBUG, "updating mode");                
                double new_balance = edit_trans_model->data(edit_trans_model->index(index_1.row(), BALANCE)).toDouble();
                QString mode = edit_trans_model->data(edit_trans_model->index(index_1.row(), MODE)).toString();
                bool negative_result = false;
                QVector<double> results;
                for(int i = index_1.row(); i < edit_trans_model->rowCount(); i++){
                    negative_result = false;
                    double trans_amount = edit_trans_model->data(edit_trans_model->index(i, TRANSACTION_AMOUNT)).toDouble();
                    if(changed_data.toString() == "Deposit"){
                        //withdraw -> deposit.
                        if(i == index_1.row()){
                            new_balance += (2 * trans_amount);
                        }else{
                            if(mode == "Deposit"){
                                new_balance += trans_amount;
                            }else{
                                if(new_balance - trans_amount < 0){
                                    QMessageBox::information(edit_trans_view, "Error", "Resulting calculation is negative, reverting all changes...");
                                    logger->log(Logger::DEBUG, "resulting calculation negative, reverting all");                                    
                                    negative_result = true;
                                    break;
                                }
                                new_balance -= trans_amount;
                            }
                        }
                        //deposit -> withdraw
                    }else{
                        if(i == index_1.row()){
                            if(new_balance - (2 * trans_amount) < 0){
                                logger->log(Logger::DEBUG, "resulting calculation negative, reverting all");                                
                                QMessageBox::information(edit_trans_view, "Error", "Resulting calculation is negative, reverting all changes...");
                                negative_result = true;
                                break;
                            }
                            new_balance -= (2 * trans_amount);
                        }else{
                            if(mode == "Deposit"){
                                new_balance += trans_amount;
                            }else{
                                if(new_balance - trans_amount < 0){
                                    logger->log(Logger::DEBUG, "resulting calculation negative, reverting all");                                    
                                    QMessageBox::information(edit_trans_view, "Error", "Resulting calculation is negative, reverting all changes...");
                                    negative_result = true;
                                    break;
                                }
                                new_balance -= trans_amount;
                            }
                        }
                    }
                    
                    results.append(new_balance);
                    
                    if(i+1 < edit_trans_model->rowCount()){
                        mode = edit_trans_model->data(edit_trans_model->index(i+1, MODE)).toString();
                    }else{
                        qDebug() << "should be exiting now...";
                    }
                }
                if(!negative_result){
                    //write to db.
                    for(int i = index_1.row(); i < results.size(); i++){
                        QModelIndex index = edit_trans_model->index(i, BALANCE, QModelIndex());
                        bool result = edit_trans_model->setData(index, results[i]);
                        logger->log(Logger::DEBUG, "update mode: " + (result ? QString("True") : QString("False")));                                
                    }
                    
                    //error checking
                    bool result = edit_trans_model->submitAll();
                    logger->log(Logger::DEBUG, "submit all mode changes: " + (result ? QString("True") : QString("False")));                                                    
                }else{
                    if(edit_trans_model->isDirty()){
                        logger->log(Logger::DEBUG, "resulting calculation negative reverting all changes");
                        QMessageBox::information(edit_trans_view, "Error", "Resulting calculation is negative, reverting all changes...");                        
                        edit_trans_model->revertAll();
                    }
                }
                ui->labelTotal->setText("Total: " + format.toCurrencyString(new_balance));
            }else{
                edit_trans_model->revertRow(index_1.row());
            }
        }else{
            logger->log(Logger::DEBUG, changed_data.toString() + " is not a valid mode");
            QMessageBox::information(edit_trans_view, "Invalid Mode", changed_data.toString() + " is not a valid mode \nChoose from [Deposit, Withdraw] (case-sensitive)");
            edit_trans_model->revertRow(index_1.row());
        }
        break;
    }
    case 3:{
        double new_amount = changed_data.toDouble();
        if(new_amount <= 0){
            logger->log(Logger::DEBUG, "invalid input editing trans amount");
            QMessageBox::information(edit_trans_view, "Invalid Input" , "Input must be greater than zero!");
            edit_trans_model->revertRow(index_1.row());
            break;
        }
        edit_trans_model->revertRow(index_1.row());
        double new_balance = edit_trans_model->data(edit_trans_model->index(index_1.row(), BALANCE)).toDouble();
        double trans_amount = edit_trans_model->data(edit_trans_model->index(index_1.row(), TRANSACTION_AMOUNT)).toDouble();
        QString mode = edit_trans_model->data(edit_trans_model->index(index_1.row(), MODE)).toString();
        bool negative_result = false;
        if(mode == "Deposit"){
            if(new_amount > trans_amount){
                new_balance += (new_amount - trans_amount);
            }else if(new_amount < trans_amount){
                new_balance -= (trans_amount - new_amount);
            }
        }else{
            if(new_amount > trans_amount){
                new_balance -= (new_amount - trans_amount);
            }else if(new_amount < trans_amount){
                new_balance += (trans_amount - new_amount);
            }
        }
        
        QVector<double> results;
        results.append(new_balance);
        
        if(new_balance < 0){
            negative_result = true;
        }
        if(!negative_result){ 
            for(int i = index_1.row() + 1; i < edit_trans_model->rowCount(); i++){
                trans_amount = edit_trans_model->data(edit_trans_model->index(i, TRANSACTION_AMOUNT)).toDouble();
                mode = edit_trans_model->data(edit_trans_model->index(i, MODE)).toString();
                if(mode == "Deposit"){
                    new_balance += trans_amount;
                }else{
                    if(new_balance - trans_amount < 0){
                        negative_result = true;
                        break;
                    }
                    new_balance -= trans_amount;
                }
                results.append(new_balance);
            }
        }
        
        if(negative_result){
            logger->log(Logger::DEBUG, "resulting calculation is negative, reverting all changes");
            if(edit_trans_model->isDirty()){
                edit_trans_model->revertAll();
            }
            QMessageBox::information(edit_trans_view, "Error", "Resulting calculation is negative, reverting all changes...");                
        }else{
            QModelIndex index = edit_trans_model->index(index_1.row(), TRANSACTION_AMOUNT, QModelIndex());
            bool result = edit_trans_model->setData(index, new_amount);
            logger->log(Logger::DEBUG, "update trans amount: " + (result ? QString("True") : QString("False")));                                            
            int i = index_1.row();
            while(!results.empty()){
                QModelIndex index = edit_trans_model->index(i, BALANCE, QModelIndex());
                bool result = edit_trans_model->setData(index, results.front());
                logger->log(Logger::DEBUG, "update balances after editing trans amount: " + (result ? QString("True") : QString("False")));                                                
                results.pop_front();
                i++;
            }
            bool status = edit_trans_model->submitAll();
            logger->log(Logger::DEBUG, "submitting all editing trans amount status: " + (status ? QString("True") : QString("False")));                                            
            ui->labelTotal->setText("Total: " + format.toCurrencyString(new_balance));
        }
        break;
    }
    case 4:{
        break;
    }
    case 5:{
        if(changed_data.toDate().isValid()){
            if(edit_trans_model->submitAll()){
                logger->log(Logger::DEBUG, "date successfully updated");
                QMessageBox::information(edit_trans_view, "Success", "Date successfully updated");
            }else{
                logger->log(Logger::DEBUG, "error updating date");
                QMessageBox::warning(edit_trans_view, "Error", "Error updating date, please try again");
            }
        }else{
            logger->log(Logger::DEBUG, "invalid date " + changed_data.toString());
            QMessageBox::information(edit_trans_view, "Invalid Date", changed_data.toString() + " is not a valid date");
            edit_trans_model->revertRow(index_1.row());
        }
        break;
    }
    default:{
        break;
    }
    }
    connect(edit_trans_model, SIGNAL(dataChanged(QModelIndex,QModelIndex)), this, SLOT(record_changed(QModelIndex,QModelIndex)));
    close_database();
}
