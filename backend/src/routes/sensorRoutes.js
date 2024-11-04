const express = require("express");
const router = express.Router();
const {
  handleUpdateRelayState,
  queryBatteryData,
} = require("../controllers/sensorController");

router.post("/sensor-data", (req, res) => {
  const io = req.app.get("io");
  const sensorData = req.body;
  require("../controllers/sensorController").handleSensorData(sensorData, io);
  res.sendStatus(201);
});

router.post("/update-relay-state", (req, res) => {
  const relayState = req.body.relayState;
  handleUpdateRelayState(relayState);
  res.sendStatus(200);
});

router.post("/query", async (req, res) => {
  const { batteryNumber, startTime, endTime } = req.body;
  try {
    const data = await queryBatteryData(batteryNumber, startTime, endTime);
    res.json(data);
  } catch (error) {
    res.status(500).json({ error: error.message });
  }
});

module.exports = router;
