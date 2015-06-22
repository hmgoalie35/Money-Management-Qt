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

#define MESSAGE_DISPLAY_LENGTH 4000

MainWindow::MainWindow(QWidget *parent) :
    QMainWindow(parent),
    ui(new Ui::MainWindow)
{
    ui->setupUi(this);
    setWindowIcon(QIcon(":/imgs/money_management.gif"));
    setWindowTitle("Money Management");
    setFixedSize(width(), height());
    ui->dateEdit->setDate(QDate::currentDate());
    ui->comboBoxMode->addItem("Deposit");
    ui->comboBoxMode->addItem("Withdraw");
    ui->comboBoxMode->setCurrentIndex(-1);
    ui->lineEditDepWithdr->setValidator(new QDoubleValidator(1, 10000000, 2, ui->lineEditDepWithdr));
    db_path = QDir::currentPath() + "/transaction_db.db";
    qDebug() << "DB Path: " << db_path;
    edit_trans_model = NULL;
    edit_trans_view = NULL;
    view_all_transactions_model = NULL;
    view_all_transactions_view = NULL;
    
    setup_database();
    open_database();
    QSqlQuery qry = transaction_db.exec("SELECT balance FROM transactions ORDER BY id DESC LIMIT 1;");
    qDebug() << "Initial Setup Query Last Balance Status: " << qry.lastError();
    if(qry.next()){
        qDebug() << "Initially setting total label to: " << format.toCurrencyString(qry.value(0).toDouble());
        ui->labelTotal->setText("Total: " + format.toCurrencyString(qry.value(0).toDouble()));
    }else{
        qDebug() << "Initially setting total label to: " << format.toCurrencyString(0.00);
        ui->labelTotal->setText("Total: " + format.toCurrencyString(0.00));
    }
    close_database();
    
}

MainWindow::~MainWindow()
{
    //    QDir d;
    //    d.remove(db_path);
    delete ui;
}
void MainWindow::setup_database(){
    transaction_db = QSqlDatabase::addDatabase("QSQLITE");
    transaction_db.setDatabaseName(db_path);
    QFileInfo db_info(db_path);
    bool exists = db_info.exists();
    bool status = transaction_db.open();
    qDebug() << "DB Exists: " << exists;
    qDebug() << "DB Opened Successfully: " << status;
    if(!status){
        qDebug() << "Error connecting to database (setup_database)";
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
    ui->statusBar->showMessage("Connected...", MESSAGE_DISPLAY_LENGTH);
    if(!exists){
        qDebug() << "Creating transaction table";
        QSqlQuery create_transaction_table_qry = transaction_db.exec("CREATE TABLE transactions(id INTEGER PRIMARY KEY AUTOINCREMENT, description TEXT, mode TEXT, trans_amount DOUBLE, balance DOUBLE, date_added DATE);");
        qDebug() << "Create Trans Table Status: " << create_transaction_table_qry.lastError();
    }
    close_database();
}

bool MainWindow::open_database(){
    bool status = transaction_db.open();
    if(status){
        ui->statusBar->showMessage("Connected...", MESSAGE_DISPLAY_LENGTH);
    }else{
        qDebug() << "Error opening database";
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

void MainWindow::close_database(){
    //  bool status = transaction_db.commit();
    //  qDebug() << "Closing database: " << status;
    transaction_db.close();
}

void MainWindow::on_actionAbout_triggered()
{
}

void MainWindow::on_pushButtonSubmit_clicked()
{
    if(!open_database()){
        QMessageBox::critical(this, "Error Saving Transaction", "Error saving the transaction, please try again");
        qDebug() << "Error connecting to database (submit button)\n Transaction not saved";
        return;
    }
    QString description = ui->lineEditDescription->text();
    QString mode = ui->comboBoxMode->currentText();
    double amount = ui->lineEditDepWithdr->text().toDouble();
    
    if(amount == 0 || description == "" || mode == ""){
        QMessageBox::critical(this, "Invalid Input", "Please provide a description, amount and if this transaction is a deposit or withdrawal.");
        qDebug() << "Invalid Input\nAmount: " << amount << " Description: " << description << " Mode: " << mode;
        return;
    }
    QSqlQuery last_trans = transaction_db.exec("SELECT balance FROM transactions ORDER BY id DESC LIMIT 1;");
    qDebug() << "Get last balance qry psh btn submit clicked status: " << last_trans.lastError();
    double last_balance = 0;
    if(last_trans.next()){
        qDebug() << "Checking last known balance (this should print once)";
        last_balance = last_trans.value(0).toDouble();
    }
    qDebug() << "last known balance (submit btn pressed): " << last_balance;
    if(mode == "Deposit"){
        qDebug() << "Depositing " << amount << " to " << last_balance;
        last_balance += amount;
        ui->labelTotal->setText("Total: " + format.toCurrencyString(last_balance));
    }else if(mode == "Withdraw"){
        qDebug() << "Withdrawing " << amount << " from " << last_balance;
        if(last_balance - amount < 0){
            qDebug() << "Can't withdraw " << amount << " from " << last_balance;
            QMessageBox::information(this, "Insufficient Funds", "There is not enough money to withdraw " + format.toCurrencyString(amount));
            return;
        }
        last_balance -= amount;
        ui->labelTotal->setText("Total: " + format.toCurrencyString(last_balance));
    }
    QSqlQuery add_transaction_qry;
    add_transaction_qry.prepare("INSERT INTO transactions (description, mode, trans_amount, balance, date_added) VALUES (:desc, :mode, :trans_amount, :balance, :date);");
    add_transaction_qry.bindValue(":desc", description);
    add_transaction_qry.bindValue(":mode", mode);
    add_transaction_qry.bindValue(":trans_amount", amount);
    add_transaction_qry.bindValue(":balance", last_balance);
    add_transaction_qry.bindValue(":date", ui->dateEdit->date());
    bool result = add_transaction_qry.exec();
    qDebug() << "Add transaction status: " << add_transaction_qry.lastError();
    if(result){
        ui->statusBar->showMessage("Transaction saved", MESSAGE_DISPLAY_LENGTH);
        ui->lineEditDepWithdr->setText("");
        ui->lineEditDescription->setText("");
        ui->comboBoxMode->setCurrentIndex(-1);
        ui->pushButtonSubmit->setEnabled(false);
        QTimer::singleShot(1750, this, SLOT(reenable_submit_btn()));
        qDebug() << "Transaction saved";
    }else{
        ui->statusBar->showMessage("Error saving transaction", MESSAGE_DISPLAY_LENGTH);
        qDebug() << "Error saving transaction";
    }
    close_database();
}

void MainWindow::reenable_submit_btn(){
    ui->pushButtonSubmit->setEnabled(true);
}

void MainWindow::on_actionQuit_triggered()
{
    this->close();
    
}
void MainWindow::closeEvent(QCloseEvent* event){
    int result = QMessageBox::question(NULL, "Quit?", "Are you sure you want to quit?");
    if(result == QMessageBox::Yes){
        qDebug() << "Exiting, user selected yes";
        if(edit_trans_view != NULL){
            edit_trans_view->deleteLater();
        }
        if(view_all_transactions_view != NULL){
            view_all_transactions_view->deleteLater();
        }
        event->accept();
    }else{
        qDebug() << "Not exiting, user selected no";
        event->ignore();
    }
}

void MainWindow::on_actionExport_triggered()
{
    QString filename = QFileDialog::getSaveFileName(this, tr("Export Database"), QDir::currentPath(), tr("Sql File (*.sql)"));
    if(!filename.isEmpty()){
        open_database();
        QSqlQuery transactions_exist_qry = transaction_db.exec("SELECT count(id) FROM transactions;");
        qDebug() << "Transactions exist qry status " << transactions_exist_qry.lastError();
        if(transactions_exist_qry.next()){
            if(transactions_exist_qry.value(0) <=0){
                qDebug() << "there are no transactions to export";
                QMessageBox::information(this, "No Transactions Exist", "There are no transactions to export, export cancelled");
                return;
            }
        }else{
            qDebug() << "there are no transactions to export";
            QMessageBox::information(this, "No Transactions Exist", "There are no transactions to export, export cancelled");
            return;
        }
        qDebug() << "transactions exist, go ahead and export";
        QSqlQuery get_all_transactions_qry = transaction_db.exec("SELECT id, description, mode, trans_amount, balance, date_added FROM transactions;");
        qDebug() << "Get all transactions for export status: " << get_all_transactions_qry.lastError();
        int count = 0;
        QFile export_file(filename);
        if(!export_file.open(QFile::WriteOnly)){
            QMessageBox::information(this, "Error Opening File", "Error opening " + filename + " for export, please try again");
            return;
        }
        export_file.write("PRAGMA foreign_keys=OFF;\n");
        export_file.write("BEGIN TRANSACTION;\n");
        export_file.write("CREATE TABLE transactions(id INTEGER PRIMARY KEY AUTOINCREMENT, description TEXT, mode TEXT, trans_amount DOUBLE, balance DOUBLE, date_added DATE);\n");
        //need to add error checking for 0 transactions.
        while(get_all_transactions_qry.next()){
            QString write_str = "INSERT INTO transactions (id, description, mode, trans_amount, balance, date_added) VALUES (" + get_all_transactions_qry.value(0).toString() + ", '" + get_all_transactions_qry.value(1).toString() + "', '" + get_all_transactions_qry.value(2).toString() + "', " + get_all_transactions_qry.value(3).toString() + ", " + get_all_transactions_qry.value(4).toString() + ", '" + get_all_transactions_qry.value(5).toString() + "');\n";
            qDebug() << "Writing " << write_str;
            export_file.write(write_str.toUtf8());
            count++;
        }
        export_file.write("COMMIT;");
        QMessageBox::information(this, "Success", QString::number(count) + " transactions exported to " + filename);
        qDebug() << count << " transactions written to " << filename;
        export_file.flush();
        export_file.close();
        close_database();
    }
}

void MainWindow::on_actionImport_triggered()
{
    QString filename = QFileDialog::getOpenFileName(this, tr("Import Database"), QDir::currentPath(), tr("Sql File (*.sql)"));
    if(!filename.isEmpty()){
        
        //@TODO -- auto backup the datbase? -- create backups folder...
        int choice = QMessageBox::question(this, "Overwrite existing data?", "This action will overwrite any existing data, are you sure you want to continue?");
        if(choice == QMessageBox::Yes){
            open_database();
            QSqlQuery drop_table_qry = transaction_db.exec("DROP TABLE transactions;");
            qDebug() << "drop table qry result: " << drop_table_qry.lastError();
            QFile import_file(filename);
            if(!import_file.open(QFile::ReadOnly)){
                QMessageBox::information(this, "Error Opening File", "Error opening " + filename + " for import, please try again");
                return;
            }
            QTextStream in(&import_file);
            QStringList errors;
            while(!in.atEnd()){
                QString statement = in.readLine();
                QSqlQuery qry = transaction_db.exec(statement);
                if(qry.lastError().text() != " "){
                    errors.push_back("Error: " + qry.lastError().text() + "\nStatement: " + statement);
                    qDebug() << "Error: " << qry.lastError() << "\nStatement: " << statement;
                }
            }
            if(errors.empty()){
                QMessageBox::information(this, "Success", "All data successfully imported");
                QSqlQuery qry = transaction_db.exec("SELECT balance FROM transactions ORDER BY id DESC LIMIT 1;");
                if(qry.next()){
                    ui->labelTotal->setText("Total: " + format.toCurrencyString(qry.value(0).toDouble()));
                }else{
                    ui->labelTotal->setText("Total: " + format.toCurrencyString(0.00));
                }
            }else{
                QMessageBox::warning(this, QString::number(errors.size()) + " Error(s)", "Encountered " + QString::number(errors.size()) + " errors(s)");
            }
            import_file.close();
            close_database();
        }
    }
}

void MainWindow::on_actionDelete_triggered()
{
    int choice = QMessageBox::question(this, "Are you sure?", "Are you sure you want to delete the entire database? This cannot be undone");
    if(choice == QMessageBox::Yes){
        open_database();
        QSqlQuery remove_all_records_qry = transaction_db.exec("DELETE FROM transactions;");
        qDebug() << "drop table qry result: " << remove_all_records_qry.lastError();
        close_database();
        if(remove_all_records_qry.lastError().text() == " "){
            ui->statusBar->showMessage("Database successfully deleted", MESSAGE_DISPLAY_LENGTH);
        }else{
            ui->statusBar->showMessage("Error deleting database", MESSAGE_DISPLAY_LENGTH);
        }
    }
}

void MainWindow::on_actionAll_Transactions_triggered()
{
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

void MainWindow::on_actionTransaction_triggered()
{
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
    edit_trans_view->hideColumn(0);
    edit_trans_view->hideColumn(4);
    edit_trans_view->setWindowIcon(QIcon(":/imgs/money_management.gif"));
    edit_trans_view->setWindowTitle("Edit Transactions");
    edit_trans_view->resizeRowsToContents();
    edit_trans_view->resizeColumnsToContents();
    //edit_trans_view->horizontalHeader()->setStretchLastSection(true);
    edit_trans_view->setGeometry(this->x(), this->y(), 400, 350);
    edit_trans_view->show();
    
    connect(edit_trans_model, SIGNAL(dataChanged(QModelIndex,QModelIndex)), this, SLOT(record_changed(QModelIndex,QModelIndex)));
    
    //get selected row and do manipulation on it...
    //need to update every transaction's balance...
    //select all records w/ id > currently changed record. Need to account for if deposit/withdrawal. then update accordingly.
    
    //only allows changing of desc and date_added?
    
    //allow user to delete a transaction see below
    //http://www.qtforum.org/article/14578/how-can-i-get-the-current-selected-row-in-a-qsqltablemodel.html
    
    //  QItemSelectionModel *selmodel = tableview->selectionModel();
    //  QModelIndex current = selmodel->currentIndex(); // the "current" item
    
    //  close_database();
}

void MainWindow::record_changed(QModelIndex index_1, QModelIndex index_2){
    Q_UNUSED (index_2);
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
            QMessageBox::information(edit_trans_view, "Success", "Description successfully updated");
        }else{
            QMessageBox::warning(edit_trans_view, "Error", "Error updating description, please try again");
        }
        break;
    }
    case 2:{
        if(changed_data.toString() == "Deposit" || changed_data.toString() == "Withdraw"){
            choice = QMessageBox::question(edit_trans_view, "Update Subsequent Transactions?", "All subsequent transactions will have their balances updated to reflect this change, continue?");
            if(choice == QMessageBox::Yes){
                double new_balance = edit_trans_model->data(edit_trans_model->index(index_1.row(), 4)).toDouble();
                QString mode = edit_trans_model->data(edit_trans_model->index(index_1.row(), 2)).toString();
                bool negative_result = false;
                QVector<double> results;
                for(int i = index_1.row(); i < edit_trans_model->rowCount(); i++){
                    negative_result = false;
                    double trans_amount = edit_trans_model->data(edit_trans_model->index(i, 3)).toDouble();
                    qDebug() << "pb: " << new_balance << " ta: " << trans_amount;
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
                                    negative_result = true;
                                    break;
                                }
                                new_balance -= trans_amount;
                            }
                        }
                        qDebug() << "deposit result: " << new_balance;
                        //deposit -> withdraw
                    }else{
                        if(i == index_1.row()){
                            if(new_balance - (2 * trans_amount) < 0){
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
                                    QMessageBox::information(edit_trans_view, "Error", "Resulting calculation is negative, reverting all changes...");
                                    qDebug() << "here: " << i;
                                    negative_result = true;
                                    break;
                                }
                                new_balance -= trans_amount;
                            }
                        }
                        qDebug() << "withdr result: " << new_balance;
                    }
                    
                    results.append(new_balance);
                    
                    if(i+1 < edit_trans_model->rowCount()){
                        mode = edit_trans_model->data(edit_trans_model->index(i+1, 2)).toString();
                    }else{
                        qDebug() << "should be exiting now...";
                    }
                }
                if(!negative_result){
                    //write to db.
                    qDebug() << "valid: " << (results.size() == (edit_trans_model->rowCount()-index_1.row()));
                    for(int i = index_1.row(); i < results.size(); i++){
                        QModelIndex index = edit_trans_model->index(i, 4, QModelIndex());
                        edit_trans_model->setData(index, results[i]);
                        qDebug() << "setting row: " << i << "'s balance to " << results[i];
                    }
                    
                    //error checking
                    edit_trans_model->submitAll();
                }else{
                    if(edit_trans_model->isDirty()){
                        QMessageBox::information(edit_trans_view, "Error", "Resulting calculation is negative, reverting all changes...");                        
                        edit_trans_model->revertAll();
                    }
                }
                ui->labelTotal->setText("Total: " + format.toCurrencyString(new_balance));
            }else{
                edit_trans_model->revertRow(index_1.row());
            }
        }else{
            QMessageBox::information(edit_trans_view, "Invalid Mode", changed_data.toString() + " is not a valid mode \nChoose from [Deposit, Withdraw] (case-sensitive)");
            edit_trans_model->revertRow(index_1.row());
        }
        qDebug() << "quitting...";
        break;
    }
    case 3:{
        double new_amount = changed_data.toDouble();
        if(new_amount <= 0){
            QMessageBox::information(edit_trans_view, "Invalid Input" , "Input must be greater than zero!");
            edit_trans_model->revertRow(index_1.row());
            break;
        }
        edit_trans_model->revertRow(index_1.row());
        double new_balance = edit_trans_model->data(edit_trans_model->index(index_1.row(), 4)).toDouble();
        double trans_amount = edit_trans_model->data(edit_trans_model->index(index_1.row(), 3)).toDouble();
        QString mode = edit_trans_model->data(edit_trans_model->index(index_1.row(), 2)).toString();
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
                trans_amount = edit_trans_model->data(edit_trans_model->index(i, 3)).toDouble();
                mode = edit_trans_model->data(edit_trans_model->index(i, 2)).toString();
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
            if(edit_trans_model->isDirty()){
                edit_trans_model->revertAll();
            }
            QMessageBox::information(edit_trans_view, "Error", "Resulting calculation is negative, reverting all changes...");                
        }else{
            QModelIndex index = edit_trans_model->index(index_1.row(), 3, QModelIndex());
            edit_trans_model->setData(index, new_amount);
            qDebug() << "setting row: " << index_1.row()+1 << " new amt to " << new_amount;
            int i = index_1.row();
            while(!results.empty()){
                QModelIndex index = edit_trans_model->index(i, 4, QModelIndex());
                edit_trans_model->setData(index, results.front());
                qDebug() << "setting row: " << (i+1) << "'s balance to " << results.front();
                results.pop_front();
                i++;
            }
            edit_trans_model->submitAll();
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
                QMessageBox::information(edit_trans_view, "Success", "Date successfully updated");
            }else{
                QMessageBox::warning(edit_trans_view, "Error", "Error updating date, please try again");
            }
        }else{
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
}
