import React from "react";
import { Link } from "react-router-dom";
import styled from "styled-components";

const NavigationBar = () => {
  return (
    <Nav>
      <NavLink to="/">Main</NavLink>
      <NavLink to="/chart">Real-time Chart</NavLink>
      <NavLink to="/data">Data</NavLink>
    </Nav>
  );
};

export default NavigationBar;

const Nav = styled.div`
  display: flex;
  justify-content: space-around;
  background: linear-gradient(45deg, #141e30, #243b55);
  padding: 15px 20px;
  box-shadow: 0px 4px 15px rgba(0, 0, 0, 0.2);
`;

const NavLink = styled(Link)`
  color: white;
  text-decoration: none;
  font-size: 18px;
  padding: 8px 15px;
  border-radius: 10px;
  transition: background 0.3s, color 0.3s;

  &:hover {
    background: rgba(255, 255, 255, 0.1);
    color: #f0f;
  }
`;
