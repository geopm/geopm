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

    def create_frequency_map_policy(max_freq, frequency_map):
        """Create a frequency map to be consumed by the frequency map agent.

        Arguments:
        min_freq: Floor frequency for the agent
        max_freq: Ceiling frequency for the agent
        frequency_map: Dictionary mapping region names to frequencies
        """
        policy = {'FREQ_DEFAULT': max_freq, 'FREQ_UNCORE': float('nan')}
        for i, (region_name, frequency) in enumerate(frequency_map.items()):
            policy['HASH_{}'.format(i)] = geopmpy.hash.crc32_str(region_name)
            policy['FREQ_{}'.format(i)] = frequency

        return policy

    def test_agent_frequency_map(self):
        name = 'test_agent_frequency_map'
        min_freq = geopm_test_launcher.geopmread("CPUINFO::FREQ_MIN board 0")
        max_freq = geopm_test_launcher.geopmread("CPUINFO::FREQ_MAX board 0")
        sticker_freq = geopm_test_launcher.geopmread("CPUINFO::FREQ_STICKER board 0")
        freq_step = geopm_test_launcher.geopmread("CPUINFO::FREQ_STEP board 0")
        self._agent = "frequency_map"
        report_path = name + '.report'
        trace_path = name + '.trace'
        num_node = 1
        num_rank = 4
        loop_count = 5
        dgemm_bigo = 15.0
        stream_bigo = 1.0
        dgemm_bigo_jlse = 35.647
        dgemm_bigo_quartz = 29.12
        stream_bigo_jlse = 1.6225
        stream_bigo_quartz = 1.7941
        hostname = socket.gethostname()
        if hostname.endswith('.alcf.anl.gov'):
            dgemm_bigo = dgemm_bigo_jlse
            stream_bigo = stream_bigo_jlse
        elif hostname.startswith('mcfly'):
            dgemm_bigo = 28.0
            stream_bigo = 1.5
        elif hostname.startswith('quartz'):
            dgemm_bigo = dgemm_bigo_quartz
            stream_bigo = stream_bigo_quartz

        app_conf = geopmpy.io.BenchConf(name + '_app.config')
        self._tmp_files.append(app_conf.get_path())
        app_conf.set_loop_count(loop_count)
        app_conf.append_region('dgemm', dgemm_bigo)
        app_conf.append_region('stream', stream_bigo)
        app_conf.append_region('all2all', 1.0)
        app_conf.write()
        freq_map = {}
        freq_map['dgemm'] = min_freq + 2 * freq_step
        freq_map['stream'] = sticker_freq - 2 * freq_step
        freq_map['all2all'] = min_freq
        self._options = create_frequency_map_policy(max_freq, freq_map)
        agent_conf = geopmpy.io.AgentConf(name + '_agent.config', self._agent, self._options)
        self._tmp_files.append(agent_conf.get_path())
        launcher = geopm_test_launcher.TestLauncher(app_conf, agent_conf, report_path,
                                                    trace_path, region_barrier=True, time_limit=900)
        launcher.set_num_node(num_node)
        launcher.set_num_rank(num_rank)
        launcher.run(name)

        self._output = geopmpy.io.AppOutput(report_path, trace_path + '*')
        node_names = self._output.get_node_names()
        self.assertEqual(len(node_names), num_node)
        regions = self._output.get_region_names()
        for nn in node_names:
            for region_name in regions:
                region_data = self._output.get_report_data(node_name=nn, region=region_name)
                if (region_name in ['dgemm', 'stream', 'all2all']):
                    #todo verify trace frequencies
                    #todo verify agent report augment frequecies
                    msg = region_name + " frequency should be near assigned map frequency"
                    util.assertNear(self, freq_map[region_name] / sticker_freq * 100, region_data['frequency'].item(), msg=msg)

if __name__ == '__main__':
    unittest.main()
