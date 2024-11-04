const mysql = require("mysql2/promise");

const dbConfig = {
  host: "localhost",
  user: "root",
  password: "sang1130!",
  database: "sensor_data",
};

async function getConnection() {
  return await mysql.createConnection(dbConfig);
}

module.exports = getConnection;
