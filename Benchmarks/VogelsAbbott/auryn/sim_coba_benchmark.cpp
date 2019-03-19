/* 
* Copyright 2014-2017 Friedemann Zenke
*
* This file is part of Auryn, a simulation package for plastic
* spiking neural networks.
* 
* Auryn is free software: you can redistribute it and/or modify
* it under the terms of the GNU General Public License as published by
* the Free Software Foundation, either version 3 of the License, or
* (at your option) any later version.
* 
* Auryn is distributed in the hope that it will be useful,
* but WITHOUT ANY WARRANTY; without even the implied warranty of
* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
* GNU General Public License for more details.
* 
* You should have received a copy of the GNU General Public License
* along with Auryn.  If not, see <http://www.gnu.org/licenses/>.
*/

/*!\file 
 *
 * \brief Simulation code for the Vogels Abbott benchmark following Brette et al. (2007)
 *
 * This simulation implements the Vogels Abbott benchmark as suggested by
 * Brette et al. (2007) Journal of Computational Neuroscience 23: 349-398. 
 *
 * The network is based on a network by Vogels and Abbott as described in 
 * Vogels, T.P., and Abbott, L.F. (2005).  Signal propagation and logic gating
 * in networks of integrate-and-fire neurons. J Neurosci 25, 10786.
 *
 * We used this network for benchmarking Auryn against other simulators in
 * Zenke, F., and Gerstner, W. (2014). Limits to high-speed simulations of
 * spiking neural networks using general-purpose computers. Front Neuroinform
 * 8, 76.
 *
 * See build/release/run_benchmark.sh for automatically run benchmarks to
 * compare the performance of different Auryn builds.
 *
 * */

// This file is re-used by the SNNSimulatorComparison Repository for benchmarking

#include "auryn.h"
#include <fstream>
#include <string>
#include <iostream>
#include <time.h>       /* clock_t, clock, CLOCKS_PER_SEC */
#include <iomanip>      // std::setprecision

using namespace auryn;

namespace po = boost::program_options;

int main(int ac,char *av[]) {
  string dir = "/tmp";

  string fwmat_ee = "../ee.wmat";
  string fwmat_ei = "../ei.wmat";
  string fwmat_ie = "../ie.wmat";
  string fwmat_ii = "../ii.wmat";

  string save = "";

  std::stringstream oss;
  string strbuf ;
  string msg;

  double w = 0.4; // [g_leak]
  double wi = 5.1; // [g_leak]



  double sparseness = 0.02;
  double simtime = 20.;
  int networkscale = 1;

  bool fast = false;

  int num_timesteps_delay = 1;

  int errcode = 0;


    try {

        po::options_description desc("Allowed options");
        desc.add_options()
            ("help", "produce help message")
            ("simtime", po::value<double>(), "simulation time")
            ("networkscale", po::value<int>(), "Network Scale, relative to 4000 Neurons")
            ("save", po::value<string>(), "Name for Network Saving")
            ("num_timesteps_delay", po::value<int>(), "the number of timesteps of synaptic delay")
            ("fast", "turns off most monitoring to reduce IO")
            ("dir", po::value<string>(), "load/save directory")
            ("fee", po::value<string>(), "file with EE connections")
            ("fei", po::value<string>(), "file with EI connections")
            ("fie", po::value<string>(), "file with IE connections")
            ("fii", po::value<string>(), "file with II connections")
        ;

        po::variables_map vm;        
        po::store(po::parse_command_line(ac, av, desc), vm);
        po::notify(vm);    

        if (vm.count("help")) {
            std::cout << desc << "\n";
            return 1;
        }

        if (vm.count("simtime")) {
          simtime = vm["simtime"].as<double>();
        } 
        if (vm.count("networkscale")) {
          networkscale = vm["networkscale"].as<int>();
          printf("Multiplying the Network Size (and dividing connectivity) by; %d\n", networkscale);
        } 
        
        if (vm.count("num_timesteps_delay")) {
          num_timesteps_delay = vm["num_timesteps_delay"].as<int>();
        } 

        if (vm.count("fast")) {
          fast = true;
        } 
        
        if (vm.count("save")) {
          save = vm["save"].as<string>();
        } 

        if (vm.count("dir")) {
          dir = vm["dir"].as<string>();
        } 

        if (vm.count("fee")) {
          fwmat_ee = vm["fee"].as<string>();
        } 

        if (vm.count("fie")) {
          fwmat_ie = vm["fie"].as<string>();
        } 

        if (vm.count("fei")) {
          fwmat_ei = vm["fei"].as<string>();
        } 

        if (vm.count("fii")) {
          fwmat_ii = vm["fii"].as<string>();
        } 

    }
    catch(std::exception& e) {
        std::cerr << "error: " << e.what() << "\n";
        return 1;
    }
    catch(...) {
        std::cerr << "Exception of unknown type!\n";
    }


  // Adjusting the sparseness and neuron number based upon scale
  sparseness /= (float)networkscale;
  printf("Network connections are %f sparse.\n", sparseness);
  NeuronID ne = 3200*networkscale;
  NeuronID ni = 800*networkscale;

  auryn_init( ac, av, dir );
  oss << dir  << "/coba." << sys->mpi_rank() << ".";
  string outputfile = oss.str();
  if ( fast ) sys->quiet = true;


  logger->msg("Setting up neuron groups ...",PROGRESS,true);

  TIFGroup * neurons_e = new TIFGroup( ne);
  TIFGroup * neurons_i = new TIFGroup( ni);
  
  neurons_e->set_delay(num_timesteps_delay);
  neurons_i->set_delay(num_timesteps_delay);

  neurons_e->set_refractory_period(5.0e-3); // minimal ISI 5.1ms
  neurons_i->set_refractory_period(5.0e-3);

  neurons_e->set_state("bg_current",2e-2); // corresponding to 200pF for C=200pF and tau=20ms
  neurons_i->set_state("bg_current",2e-2);


  logger->msg("Setting up E connections ...",PROGRESS,true);


  SparseConnection * con_ee 
    = new SparseConnection(neurons_e, neurons_e, w, sparseness, GLUT);

  SparseConnection * con_ei 
    = new SparseConnection(neurons_e, neurons_i, w, sparseness, GLUT);



  logger->msg("Setting up I connections ...",PROGRESS,true);
  SparseConnection * con_ie 
    = new SparseConnection(neurons_i, neurons_e, wi, sparseness, GABA);

  SparseConnection * con_ii 
    = new SparseConnection(neurons_i, neurons_i, wi, sparseness, GABA);




  if (networkscale == 1){
    if ( !fwmat_ee.empty() ) std::cout << "Loading connectivity from file." << std::endl;
    if ( !fwmat_ee.empty() ) con_ee->load_from_complete_file(fwmat_ee);
    if ( !fwmat_ei.empty() ) con_ei->load_from_complete_file(fwmat_ei);
    if ( !fwmat_ie.empty() ) con_ie->load_from_complete_file(fwmat_ie);
    if ( !fwmat_ii.empty() ) con_ii->load_from_complete_file(fwmat_ii);
  }

  if (!save.empty()){
    sys->save_network_state_text(save); //std::to_string(networkscale));
  }



  if ( !fast ) {
    logger->msg("Use --fast option to turn off IO for benchmarking!", WARNING);

    msg = "Setting up monitors ...";
    logger->msg(msg,PROGRESS,true);

    std::stringstream filename;
    filename << outputfile << "e.ras";
    SpikeMonitor * smon_e = new SpikeMonitor( neurons_e, filename.str().c_str() );

    filename.str("");
    filename.clear();
    filename << outputfile << "i.ras";
    SpikeMonitor * smon_i = new SpikeMonitor( neurons_i, filename.str().c_str() );
  }



  // RateChecker * chk = new RateChecker( neurons_e , -0.1 , 1000. , 100e-3);
  //printf("Auryn timestep: %f", sys->auryn_timestep);
  
  logger->msg("Simulating ..." ,PROGRESS,true);
  clock_t starttime = clock();
  if (!sys->run(simtime,true)) 
      errcode = 1;
  clock_t totaltime = clock() - starttime;

  if ( fast ){
    std::ofstream timefile;
    timefile.open("timefile.dat");
    timefile << std::setprecision(10) << ((float)totaltime / CLOCKS_PER_SEC);
    timefile.close();
  }

  if ( sys->mpi_rank() == 0 ) {
    logger->msg("Saving elapsed time ..." ,PROGRESS,true);
    char filenamebuf [255];
    sprintf(filenamebuf, "%s/elapsed.dat", dir.c_str());
    std::ofstream timefile;
    timefile.open(filenamebuf);
    timefile << sys->get_last_elapsed_time() << std::endl;
    timefile.close();
  }

  if (errcode)
    auryn_abort(errcode);

  logger->msg("Freeing ..." ,PROGRESS,true);
  auryn_free();
  return errcode;
}
