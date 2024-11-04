const express = require("express");
const cors = require("cors");
const sensorRoutes = require("./routes/sensorRoutes");

const app = express();
app.use(cors());
app.use(express.json());
app.use("/api", sensorRoutes);

module.exports = app;
