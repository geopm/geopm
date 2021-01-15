
# TODO: what is the point of this test?  being skipped in nightlies.

@util.skip_unless_do_launch()
class TestIntegration(unittest.TestCase):
    def setUp(self):
        self.longMessage = True
        self._agent = 'power_governor'
        self._options = {'power_budget': 150}
        self._tmp_files = []
        self._output = None
        self._power_limit = geopm_test_launcher.geopmread("MSR::PKG_POWER_LIMIT:PL1_POWER_LIMIT board 0")
        self._frequency = geopm_test_launcher.geopmread("MSR::PERF_CTL:FREQ board 0")
        self._original_freq_map_env = os.environ.get('GEOPM_FREQUENCY_MAP')

    def tearDown(self):
        geopm_test_launcher.geopmwrite("MSR::PKG_POWER_LIMIT:PL1_POWER_LIMIT board 0 " + str(self._power_limit))
        geopm_test_launcher.geopmwrite("MSR::PERF_CTL:FREQ board 0 " + str(self._frequency))
        if self._original_freq_map_env is None:
            if 'GEOPM_FREQUENCY_MAP' in os.environ:
                os.environ.pop('GEOPM_FREQUENCY_MAP')
        else:
            os.environ['GEOPM_FREQUENCY_MAP'] = self._original_freq_map_env


    @unittest.skipUnless(geopm_test_launcher.detect_launcher() == "srun" and os.getenv('SLURM_NODELIST') is None,
                         'Requires non-sbatch SLURM session for alloc\'d and idle nodes.')
    def test_report_generation_all_nodes(self):
        name = 'test_report_generation_all_nodes'
        report_path = name + '.report'
        num_node = 1
        num_rank = 1
        delay = 1.0
        app_conf = geopmpy.io.BenchConf(name + '_app.config')
        self._tmp_files.append(app_conf.get_path())
        app_conf.append_region('sleep', delay)
        agent_conf = geopmpy.io.AgentConf(name + '_agent.config', self._agent, self._options)
        self._tmp_files.append(agent_conf.get_path())
        launcher = geopm_test_launcher.TestLauncher(app_conf, agent_conf, report_path)
        launcher.set_num_node(num_node)
        launcher.set_num_rank(num_rank)
        time.sleep(5)  # Wait a moment to finish cleaning-up from a previous test
        idle_nodes = launcher.get_idle_nodes()
        idle_nodes_copy = list(idle_nodes)
        alloc_nodes = launcher.get_alloc_nodes()
        launcher.write_log(name, 'Idle nodes : {nodes}'.format(nodes=idle_nodes))
        launcher.write_log(name, 'Alloc\'d  nodes : {nodes}'.format(nodes=alloc_nodes))
        node_names = []
        for nn in idle_nodes_copy:
            launcher.set_node_list(nn.split())  # Hack to convert string to list
            try:
                launcher.run(name)
                node_names += nn.split()
            except subprocess.CalledProcessError as e:
                if e.returncode == 1 and nn not in launcher.get_idle_nodes():
                    launcher.write_log(name, '{node} has disappeared from the idle list!'.format(node=nn))
                    idle_nodes.remove(nn)
                else:
                    launcher.write_log(name, 'Return code = {code}'.format(code=e.returncode))
                    raise e
            ao = geopmpy.io.AppOutput(report_path, do_cache=False)
            sleep_data = ao.get_report_data(node_name=nn, region='sleep')
            app_data = ao.get_app_total_data(node_name=nn)
            self.assertNotEqual(0, len(sleep_data))
            util.assertNear(self, delay, sleep_data['runtime'].item())
            self.assertGreater(app_data['runtime'].item(), sleep_data['runtime'].item())
            self.assertEqual(1, sleep_data['count'].item())

        self.assertEqual(len(node_names), len(idle_nodes))


if __name__ == '__main__':
    unittest.main()
