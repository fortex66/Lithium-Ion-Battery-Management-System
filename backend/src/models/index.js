const Sequelize = require("sequelize");
const sequelize = require("../config/db");

const SensorData = require("./sensorData")(sequelize, Sequelize.DataTypes);

const db = {
  SensorData,
  sequelize,
  Sequelize,
};

module.exports = db;
