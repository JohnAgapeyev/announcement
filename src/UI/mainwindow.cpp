#include <QDir>
#include <QInputDialog>
#include <QMessageBox>
#include <QTableWidgetItem>
#include <iostream>
#include <sstream>
#include <vector>

#include "announce.h"
#include "interactive.h"
#include "mainwindow.h"
#include "ui_mainwindow.h"

MainWindow::MainWindow(QWidget* parent) : QMainWindow(parent), ui(new Ui::MainWindow) {
    ui->setupUi(this);
}

MainWindow::~MainWindow() {
    delete ui;
}

void MainWindow::on_add_entry_clicked() {
    bool ok;
    QString beliefs = QInputDialog::getText(
            this, tr("Initial Agent Beliefs"), tr("Initial Beliefs:"), QLineEdit::Normal, "", &ok);

    if (!ok || beliefs.isEmpty()) {
        QMessageBox::information(this, tr("Announcement Solver"),
                tr("Initial agent beliefs contained invalid token"));
        return;
    }

    QString goals = QInputDialog::getText(
            this, tr("Desired Agent Goal"), tr("Desired Goal:"), QLineEdit::Normal, "", &ok);

    if (!ok || goals.isEmpty()) {
        QMessageBox::information(
                this, tr("Announcement Solver"), tr("Agent goal contained invalid token"));
        return;
    }

    std::stringstream ss{shunting_yard(beliefs.toStdString())};
    std::vector<std::string> belief_tokens{
            std::istream_iterator<std::string>{ss}, std::istream_iterator<std::string>{}};

    ss = std::stringstream{shunting_yard(goals.toStdString())};
    std::vector<std::string> goal_tokens{
            std::istream_iterator<std::string>{ss}, std::istream_iterator<std::string>{}};

    if (belief_tokens.empty() || goal_tokens.empty()) {
        QMessageBox::information(
                this, tr("Announcement Solver"), tr("Agent beliefs and/or goal was malformed"));
        return;
    }

    agent_list.emplace_back(create_agent(belief_tokens, goal_tokens));

    ui->data_table->insertRow(ui->data_table->rowCount());
    QTableWidgetItem* item = new QTableWidgetItem();
    item->setCheckState(Qt::Unchecked);
    ui->data_table->setItem(ui->data_table->rowCount() - 1, 0, item);
    ui->data_table->setItem(ui->data_table->rowCount() - 1, 1,
            new QTableWidgetItem(QString::number(ui->data_table->rowCount())));
    ui->data_table->setItem(ui->data_table->rowCount() - 1, 2, new QTableWidgetItem(beliefs));
    ui->data_table->setItem(ui->data_table->rowCount() - 1, 3, new QTableWidgetItem(goals));
}

void MainWindow::on_remove_entry_clicked() {
    for (int i = 0; i < ui->data_table->rowCount(); ++i) {
        QTableWidgetItem* item = ui->data_table->item(i, 0);
        if (item && item->checkState() == Qt::Checked) {
            ui->data_table->removeRow(i);
            agent_list.erase(agent_list.begin() + i);
            //Fix weird issue with column staying put
            --i;
        }
    }
    for (int i = 0; i < ui->data_table->rowCount(); ++i) {
        ui->data_table->item(i, 1)->setText(QString::number(i + 1));
    }
}

void MainWindow::on_calculate_announcement_clicked() {
    if (!agent_list.empty()) {
        //find_announcement(agent_list);
        QMessageBox::information(
                this, tr("Announcement Solver"), tr(find_announcement(agent_list).c_str()));
    } else {
        QMessageBox::information(this, tr("Announcement Solver"),
                tr("You have to enter an entry before you can find an announcement"));
    }
}

void MainWindow::on_generate_test_clicked() {
    QMessageBox::StandardButton sat_reply = QMessageBox::question(this, "Announcement Solver",
            "Should be the goals be satisfiable?", QMessageBox::Yes | QMessageBox::No);
    bool ok;
    int agent_count = QInputDialog::getInt(this, tr("Announcement Solver"),
            tr("Desired number of agents/beliefs"), 1, 1, 10000, 1, &ok);
    if (!ok) {
        return;
    }

    //Clear existing entries
    for (int i = 0; i < ui->data_table->rowCount(); ++i) {
        ui->data_table->removeRow(i);
        agent_list.clear();
        --i;
    }

    if (sat_reply == QMessageBox::Yes) {
        for (int i = 0; i < agent_count; ++i) {
            std::stringstream belief;
            std::stringstream goal;
            for (int j = 0; j < agent_count; ++j) {
                if (rand() % 2) {
                    belief << "not ";
                }
                if (i != j) {
                    //goal << "not ";
                }
                if (j + 1 == agent_count) {
                    belief << (j + 1);
                    //goal << (j + 1);
                } else {
                    if (rand() % 2) {
                        belief << (j + 1) << " or ";
                    } else {
                        belief << (j + 1) << " and ";
                    }
                    //goal << (j + 1) << " and ";
                }
            }
            if (rand() % 2) {
                goal << "not ";
            }
            goal << (i + 1);

            std::string belief_string = belief.str();
            std::string goal_string = goal.str();

            std::stringstream ss{shunting_yard(belief_string)};
            std::vector<std::string> belief_tokens{
                    std::istream_iterator<std::string>{ss}, std::istream_iterator<std::string>{}};

            ss = std::stringstream{shunting_yard(goal_string)};
            std::vector<std::string> goal_tokens{
                    std::istream_iterator<std::string>{ss}, std::istream_iterator<std::string>{}};

            agent_list.emplace_back(create_agent(belief_tokens, goal_tokens));

            ui->data_table->insertRow(ui->data_table->rowCount());
            QTableWidgetItem* item = new QTableWidgetItem();
            item->setCheckState(Qt::Unchecked);
            ui->data_table->setItem(ui->data_table->rowCount() - 1, 0, item);
            ui->data_table->setItem(ui->data_table->rowCount() - 1, 1,
                    new QTableWidgetItem(QString::number(ui->data_table->rowCount())));
            ui->data_table->setItem(
                    ui->data_table->rowCount() - 1, 2, new QTableWidgetItem(QString::fromStdString(belief_string)));
            ui->data_table->setItem(ui->data_table->rowCount() - 1, 3, new QTableWidgetItem(QString::fromStdString(goal_string)));
        }
    } else {
        QMessageBox::StandardButton solve_reply = QMessageBox::question(this, "Announcement Solver",
                "Should be the problem have a solution?", QMessageBox::Yes | QMessageBox::No);
        if (solve_reply == QMessageBox::Yes) {
            for (int i = 0; i < agent_count; ++i) {
                std::stringstream belief;
                std::stringstream goal;
                for (int j = 0; j < agent_count; ++j) {
                    if (rand() % 2) {
                        belief << "not ";
                        goal << "not ";
                    }
                    if (i != j) {
                        //goal << "not ";
                    }
                    if (j + 1 == agent_count) {
                        belief << (j + 1);
                        goal << (j + 1);
                    } else {
#if 0
                        if (rand() % 2) {
                            belief << (j + 1) << " or ";
                        } else {
                            belief << (j + 1) << " and ";
                        }
#endif
                        belief << (j + 1) << " or ";
                        goal << (j + 1) << " and ";
                    }
                }

                std::string belief_string = belief.str();
                std::string goal_string = goal.str();

                std::stringstream ss{shunting_yard(belief_string)};
                std::vector<std::string> belief_tokens{
                        std::istream_iterator<std::string>{ss}, std::istream_iterator<std::string>{}};

                ss = std::stringstream{shunting_yard(goal_string)};
                std::vector<std::string> goal_tokens{
                        std::istream_iterator<std::string>{ss}, std::istream_iterator<std::string>{}};

                agent_list.emplace_back(create_agent(belief_tokens, goal_tokens));

                ui->data_table->insertRow(ui->data_table->rowCount());
                QTableWidgetItem* item = new QTableWidgetItem();
                item->setCheckState(Qt::Unchecked);
                ui->data_table->setItem(ui->data_table->rowCount() - 1, 0, item);
                ui->data_table->setItem(ui->data_table->rowCount() - 1, 1,
                        new QTableWidgetItem(QString::number(ui->data_table->rowCount())));
                ui->data_table->setItem(
                        ui->data_table->rowCount() - 1, 2, new QTableWidgetItem(QString::fromStdString(belief_string)));
                ui->data_table->setItem(ui->data_table->rowCount() - 1, 3, new QTableWidgetItem(QString::fromStdString(goal_string)));
            }
        } else {
            for (int i = 0; i < agent_count; ++i) {
                std::stringstream belief;
                std::stringstream goal;

                belief << (i + 1);
                if (i & 1) {
                    goal << "not ";
                }
                goal << 1;

                std::string belief_string = belief.str();
                std::string goal_string = goal.str();

                std::stringstream ss{shunting_yard(belief_string)};
                std::vector<std::string> belief_tokens{
                        std::istream_iterator<std::string>{ss}, std::istream_iterator<std::string>{}};

                ss = std::stringstream{shunting_yard(goal_string)};
                std::vector<std::string> goal_tokens{
                        std::istream_iterator<std::string>{ss}, std::istream_iterator<std::string>{}};

                agent_list.emplace_back(create_agent(belief_tokens, goal_tokens));

                ui->data_table->insertRow(ui->data_table->rowCount());
                QTableWidgetItem* item = new QTableWidgetItem();
                item->setCheckState(Qt::Unchecked);
                ui->data_table->setItem(ui->data_table->rowCount() - 1, 0, item);
                ui->data_table->setItem(ui->data_table->rowCount() - 1, 1,
                        new QTableWidgetItem(QString::number(ui->data_table->rowCount())));
                ui->data_table->setItem(
                        ui->data_table->rowCount() - 1, 2, new QTableWidgetItem(QString::fromStdString(belief_string)));
                ui->data_table->setItem(ui->data_table->rowCount() - 1, 3, new QTableWidgetItem(QString::fromStdString(goal_string)));
            }
        }
    }
}
