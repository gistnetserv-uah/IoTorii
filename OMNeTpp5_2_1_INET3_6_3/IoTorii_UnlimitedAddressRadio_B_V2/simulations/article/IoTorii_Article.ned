//
// Copyright (C) 2018 Elisa Rojas(1), Hedayat Hosseini(2);
//                    (1) GIST, University of Alcala, Spain.
//                    (2) CEIT, Amirkabir University of Technology (Tehran Polytechnic), Iran.
//                    OMNeT++ 5.2.1 & INET 3.6.3
//
// This program is free software: you can redistribute it and/or modify
// it under the terms of the GNU Lesser General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.
// 
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU Lesser General Public License for more details.
// 
// You should have received a copy of the GNU Lesser General Public License
// along with this program.  If not, see http://www.gnu.org/licenses/.
// 

package iotorii.simulations.article;

import inet.networklayer.configurator.ipv6.FlatNetworkConfigurator6;
//import inet.physicallayer.ieee802154.packetlevel.Ieee802154NarrowbandScalarRadioMedium;
import iotorii.src.physicallayer.simpleidealradio.SimpleIdealRadioMedium;
import iotorii.src.node.inet.WSNHostIoTorii;
import iotorii.src.statisticcollector.StatisticCollector;

network Net802154
{
    parameters:
        int numHosts;
    submodules:
        configurator: FlatNetworkConfigurator6 {
            parameters:
                @display("p=19,66");
        }
        radioMedium: SimpleIdealRadioMedium {
            parameters:
                @display("p=73,17");
        }
        statisticCollector: StatisticCollector {
            parameters:
                @display("p=19,17");
        }
        host[numHosts]: WSNHostIoTorii {
            parameters:
                @display("r=,,#707070;p=146,58");
        }
}
