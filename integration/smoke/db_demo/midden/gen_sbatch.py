#!/usr/bin/env python

import textwrap
import argparse

# TODO: needs to use launcher timeout so that smoke.py has a chance to run if test times out


def gen_sbatch(app, exp_type, node_count):

    exp_dir = exp_type
    if exp_type in ['barrier_frequency_sweep', 'power_balancer_energy']:
        exp_dir = 'energy_efficient'

    args = ""
    if exp_type in ['frequency_sweep', 'barrier_frequency_sweep']:
        args = "--min-frequency=1.9e9 --max-frequency=2.0e9"
    elif exp_type in ['power_sweep', 'power_balancer_energy']:
        args = "--min-power=220 --max-power=230"
    elif exp_type in ['uncore_frequency_sweep']:
        args = "--min-frequency=1.9e9 --max-frequency=2.0e9 --min-uncore-frequency=2.1e9 --max-uncore-frequency=2.2e9"

    template = textwrap.dedent('''\
    #!/bin/bash
    #SBATCH -N {node_count}
    #SBATCH -t 4:00:00
    #SBATCH -o %j.out

    GEOPM_SRC=$HOME/geopm
    GEOPM_EXP=$GEOPM_SRC/integration/experiment

    source ~/env.sh
    source ~/flask/venv/bin/activate

    OUTDIR=${{SLURM_JOB_ID}}_{app}_{exp_type}

    SCRIPT=$GEOPM_EXP/{exp_dir}/run_{exp_type}_{app}.py
    if [ -f $SCRIPT ]; then
        $SCRIPT --output-dir=$OUTDIR \\
        --node-count=$SLURM_NNODES \\
        --trial-count=1 \\
        {args} \\
        # end

        result=$?
        if [ $result -eq 0 ]; then
            {smoke_script} --jobid=$SLURM_JOB_ID --app={app} --exp-type={exp_type} --result="PASS"
        else
            {smoke_script} --jobid=$SLURM_JOB_ID --app={app} --exp-type={exp_type} --result="FAIL"
        fi
    else
        {smoke_script} --jobid=$SLURM_JOB_ID --app={app} --exp-type={exp_type} --result="No script"
    fi

    '''.format(exp_dir=exp_dir, exp_type=exp_type, app=app, args=args, node_count=node_count,
               smoke_script="${GEOPM_SRC}/integration/smoke/db_demo/smoke.py"))

    filename = '{}_{}.sbatch'.format(app, exp_type)

    with open(filename, 'w') as outfile:
        outfile.write(template)


if __name__ == '__main__':

    parser = argparse.ArgumentParser()
    parser.add_argument('--app', type=str, required=True,
                        help='application name')
    parser.add_argument('--exp-type', type=str, dest='exp_type', required=True,
                        help='experiment type')
    parser.add_argument('--node-count', type=int, dest='node_count', required=True, help='node count')
    args = parser.parse_args()

    app_name = args.app
    exp_type = args.exp_type
    node_count = args.node_count

    gen_sbatch(app=app_name, exp_type=exp_type, node_count=node_count)
