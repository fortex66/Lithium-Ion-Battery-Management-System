import React from "react";
import {
  LineChart,
  Line,
  XAxis,
  YAxis,
  CartesianGrid,
  Tooltip,
  Legend,
} from "recharts";
import styled from "styled-components";

const ChartPage = ({ data }) => {
  const formatXAxis = (tickItem) => {
    return new Date(tickItem * 1000).toISOString().substr(14, 5);
  };

  const renderChart = (idx) => (
    <LineChart
      key={idx}
      width={1200}
      height={600}
      data={data.times.map((time, index) => ({
        time,
        Volt: data.voltages[idx][index],
        Current: data.currents[idx][index],
        Temp:
          data.relayState[idx] === 0
            ? data.temperatures[idx][index]
            : data.resisterTemps[idx][index],
        PWM: data.duty_cycles[idx][index],
      }))}
      margin={{ top: 5, right: 30, left: 30, bottom: 30 }}
    >
      <CartesianGrid strokeDasharray="3 3" />
      <XAxis dataKey="time" tickFormatter={formatXAxis} />
      <YAxis
        yAxisId="voltage"
        orientation="left"
        stroke="#8884d8"
        domain={[0, 5]}
        width={50}
      />
      <YAxis
        yAxisId="current"
        orientation="left"
        stroke="#82ca9d"
        domain={[-2, 2]}
        width={50}
      />
      <YAxis
        yAxisId="temperature"
        orientation="right"
        stroke="#ff7300"
        domain={[0, 50]}
        width={50}
      />
      <YAxis
        yAxisId="pwm"
        orientation="right"
        stroke="#ff33cc"
        domain={[0, 100]}
        width={50}
      />
      <Tooltip />
      <Legend />
      <Line yAxisId="voltage" type="monotone" dataKey="Volt" stroke="#8884d8" />
      <Line
        yAxisId="current"
        type="monotone"
        dataKey="Current"
        stroke="#82ca9d"
      />
      <Line
        yAxisId="temperature"
        type="monotone"
        dataKey="Temp"
        stroke={data.relayState[idx] === 0 ? "#ff7300" : "#66"} // 색상 조건부 변경
      />
      <Line yAxisId="pwm" type="monotone" dataKey="PWM" stroke="#ff33cc" />
    </LineChart>
  );

  return (
    <Container>
      <h2>Battery Data Charts</h2>
      <ChartsContainer>
        {data.voltages.map((_, idx) => (
          <ChartWrapper key={idx}>
            <h3>Battery {idx + 1}</h3>
            {renderChart(idx)}
          </ChartWrapper>
        ))}
      </ChartsContainer>
    </Container>
  );
};

export default ChartPage;

const Container = styled.div`
  display: flex;
  flex-direction: column;
  align-items: center;
  margin: 20px;
`;

const ChartsContainer = styled.div`
  display: flex;
  flex-direction: column; /* 세로 배치를 위해 수정 */
  align-items: center;
  width: 100%;
`;

const ChartWrapper = styled.div`
  display: flex;
  flex-direction: column;
  align-items: center;
  margin: 20px 0; /* 세로 간격 조정 */
`;
