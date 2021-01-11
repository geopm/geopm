-- phpMyAdmin SQL Dump
-- version 4.4.15.10
-- https://www.phpmyadmin.net
--
-- Host: 127.0.0.1
-- Generation Time: Oct 16, 2020 at 01:21 AM
-- Server version: 5.5.60-MariaDB
-- PHP Version: 5.4.16

SET SQL_MODE = "NO_AUTO_VALUE_ON_ZERO";
SET time_zone = "+00:00";


/*!40101 SET @OLD_CHARACTER_SET_CLIENT=@@CHARACTER_SET_CLIENT */;
/*!40101 SET @OLD_CHARACTER_SET_RESULTS=@@CHARACTER_SET_RESULTS */;
/*!40101 SET @OLD_COLLATION_CONNECTION=@@COLLATION_CONNECTION */;
/*!40101 SET NAMES utf8mb4 */;

--
-- Database: `test`
--

-- --------------------------------------------------------

--
-- Table structure for table `ExperimentResults`
--

CREATE TABLE IF NOT EXISTS `ExperimentResults` (
  `id` int(11) NOT NULL,
  `experiment_id` int(11) NOT NULL,
  `policy_id` int(11) NOT NULL,
  `label` text NOT NULL,
  `value` double NOT NULL
) ENGINE=InnoDB DEFAULT CHARSET=latin1;

-- --------------------------------------------------------

--
-- Table structure for table `Experiments`
--

CREATE TABLE IF NOT EXISTS `Experiments` (
  `id` int(11) NOT NULL,
  `output_dir` text NOT NULL,
  `jobid` int(11) NOT NULL,
  `type` text NOT NULL,
  `date` date NOT NULL,
  `notes` text NOT NULL,
  `raw_data` longblob NOT NULL
) ENGINE=InnoDB DEFAULT CHARSET=latin1;

-- --------------------------------------------------------

--
-- Table structure for table `Policies`
--

CREATE TABLE IF NOT EXISTS `Policies` (
  `id` int(11) NOT NULL,
  `agent` text NOT NULL
) ENGINE=InnoDB DEFAULT CHARSET=latin1;

-- --------------------------------------------------------

--
-- Table structure for table `PolicyParamLink`
--

CREATE TABLE IF NOT EXISTS `PolicyParamLink` (
  `id` int(11) NOT NULL,
  `policy_id` int(11) NOT NULL,
  `param_id` int(11) NOT NULL
) ENGINE=InnoDB DEFAULT CHARSET=latin1;

-- --------------------------------------------------------

--
-- Table structure for table `PolicyParams`
--

CREATE TABLE IF NOT EXISTS `PolicyParams` (
  `id` int(11) NOT NULL,
  `name` text NOT NULL,
  `value` double NOT NULL
) ENGINE=InnoDB DEFAULT CHARSET=latin1;

-- --------------------------------------------------------

--
-- Table structure for table `Reports`
--

CREATE TABLE IF NOT EXISTS `Reports` (
  `id` int(11) NOT NULL,
  `experiment_id` int(11) NOT NULL,
  `agent` text NOT NULL,
  `profile_name` text NOT NULL,
  `geopm_version` text NOT NULL,
  `start_time` date NOT NULL,
  `policy_id` int(11) NOT NULL,
  `total_runtime` double NOT NULL,
  `average_power` double NOT NULL,
  `figure_of_merit` double NOT NULL
) ENGINE=InnoDB DEFAULT CHARSET=latin1;

--
-- Indexes for dumped tables
--

--
-- Indexes for table `ExperimentResults`
--
ALTER TABLE `ExperimentResults`
  ADD PRIMARY KEY (`id`),
  ADD KEY `experiment_id` (`experiment_id`),
  ADD KEY `policy_id` (`policy_id`);

--
-- Indexes for table `Experiments`
--
ALTER TABLE `Experiments`
  ADD PRIMARY KEY (`id`),
  ADD KEY `jobid` (`jobid`);

--
-- Indexes for table `Policies`
--
ALTER TABLE `Policies`
  ADD PRIMARY KEY (`id`);

--
-- Indexes for table `PolicyParamLink`
--
ALTER TABLE `PolicyParamLink`
  ADD PRIMARY KEY (`id`),
  ADD KEY `policy_id` (`policy_id`),
  ADD KEY `param_id` (`param_id`);

--
-- Indexes for table `PolicyParams`
--
ALTER TABLE `PolicyParams`
  ADD PRIMARY KEY (`id`);

--
-- Indexes for table `Reports`
--
ALTER TABLE `Reports`
  ADD PRIMARY KEY (`id`),
  ADD KEY `experiment_id` (`experiment_id`),
  ADD KEY `policy_id` (`policy_id`);

--
-- AUTO_INCREMENT for dumped tables
--

--
-- AUTO_INCREMENT for table `ExperimentResults`
--
ALTER TABLE `ExperimentResults`
  MODIFY `id` int(11) NOT NULL AUTO_INCREMENT;
--
-- AUTO_INCREMENT for table `Experiments`
--
ALTER TABLE `Experiments`
  MODIFY `id` int(11) NOT NULL AUTO_INCREMENT;
--
-- AUTO_INCREMENT for table `Policies`
--
ALTER TABLE `Policies`
  MODIFY `id` int(11) NOT NULL AUTO_INCREMENT;
--
-- AUTO_INCREMENT for table `PolicyParamLink`
--
ALTER TABLE `PolicyParamLink`
  MODIFY `id` int(11) NOT NULL AUTO_INCREMENT;
--
-- AUTO_INCREMENT for table `PolicyParams`
--
ALTER TABLE `PolicyParams`
  MODIFY `id` int(11) NOT NULL AUTO_INCREMENT;
--
-- AUTO_INCREMENT for table `Reports`
--
ALTER TABLE `Reports`
  MODIFY `id` int(11) NOT NULL AUTO_INCREMENT;
--
-- Constraints for dumped tables
--

--
-- Constraints for table `ExperimentResults`
--
ALTER TABLE `ExperimentResults`
  ADD CONSTRAINT `result_policy_id` FOREIGN KEY (`policy_id`) REFERENCES `Policies` (`id`),
  ADD CONSTRAINT `result_experiment_id` FOREIGN KEY (`experiment_id`) REFERENCES `Experiments` (`id`);

--
-- Constraints for table `PolicyParamLink`
--
ALTER TABLE `PolicyParamLink`
  ADD CONSTRAINT `link_policy_id` FOREIGN KEY (`policy_id`) REFERENCES `Policies` (`id`),
  ADD CONSTRAINT `link_param_id` FOREIGN KEY (`param_id`) REFERENCES `PolicyParams` (`id`);

--
-- Constraints for table `Reports`
--
ALTER TABLE `Reports`
  ADD CONSTRAINT `report_policy_id` FOREIGN KEY (`policy_id`) REFERENCES `Policies` (`id`),
  ADD CONSTRAINT `report_experiment_id` FOREIGN KEY (`experiment_id`) REFERENCES `Experiments` (`id`);

/*!40101 SET CHARACTER_SET_CLIENT=@OLD_CHARACTER_SET_CLIENT */;
/*!40101 SET CHARACTER_SET_RESULTS=@OLD_CHARACTER_SET_RESULTS */;
/*!40101 SET COLLATION_CONNECTION=@OLD_COLLATION_CONNECTION */;
