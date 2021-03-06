/*
 * The implementation of HLMAC address is inspired by the MAC address implementation.
 * Copyright (C) 2003 Andras Varga; CTIE, Monash University, Australia
*/

/*
 * Copyright (C) 2017 Elisa Rojas(1), SeyedHedayat Hosseini(2);
 *                    (1) GIST, University of Alcala, Spain.
 *                    (2) CEIT, Amirkabir University of Technology (Tehran Polytechnic), Iran.
 *
 * Main paper:
 * Rojas, Elisa, et al. "GA3: scalable, distributed address assignment
 * for dynamic data center networks." Annals of Telecommunications (2017): 1-10.�
 * DOI: http://dx.doi.org/10.1007/s12243-017-0569-4
 *
 * Developed in OMNet++5.2, based on INET framework.
 * LAST UPDATE OF THE INET FRAMEWORK: inet3.6.2 @ October 2017
*/

/*
 * Copyright (C) 2018 Elisa Rojas(1), Hedayat Hosseini(2);
 *                    (1) GIST, University of Alcala, Spain.
 *                    (2) CEIT, Amirkabir University of Technology (Tehran Polytechnic), Iran.
 *                    OMNeT++ 5.2.1 & INET 3.6.3
*/

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

#ifndef IOTORII_SRC_LINKLAYER_COMMON_HLMACADDRESS_H_
#define IOTORII_SRC_LINKLAYER_COMMON_HLMACADDRESS_H_

#define HLMAC_ADDRESS_SIZE    2   //2 bytes, short address (16 bits)
#define HLMAC_WIDTH 2  // 2 bits, HLMAC_LENGTH = HLMAC_ADDRESS_SIZE . 8 / HLMAC_WIDTH -1. note that -1 is the space for saving HLMAC Type
#define HLMAC_ADDRESS_MASK    0b1111111111111100ULL  //The number of bits is based on HLMAC_ADDRESS_SIZE * 8, the number of zeros is based on the HLMAC_WIDTH

#include <string>
#include "inet/common/INETDefs.h"

namespace iotorii {
using namespace inet;

class HLMACAddress {

private:
  uint64 address;

public:

  /** The unspecified HLMAC address, 0.0.0.0.0.0.0 */
  static const HLMACAddress UNSPECIFIED_ADDRESS;

  /** The broadcast HLMAC address, 3.3.3.3.3.3.3 */
  static const HLMACAddress BROADCAST_ADDRESS;

  /**
   * Default constructor initializes address bytes to zero.
   */
  HLMACAddress() { address = 0; }

  /**
   * Initializes the address from the lower 48 bits of the 64-bit argument
   */
  explicit HLMACAddress(uint64 bits) { address = bits & HLMAC_ADDRESS_MASK;}

  /**
   * Constructor which accepts a hex string (7 hex digits (in this case), may also
   * contain spaces, hyphens and colons)
   */
  explicit HLMACAddress(const char *hexstr) { setAddress(hexstr); }

  /**
   * Copy constructor.
   */
  HLMACAddress(const HLMACAddress& other) { address = other.address;}

  /**
   * Returns the address width.
   */
  unsigned int getHLMACWidth() const { return HLMAC_WIDTH; }

  /**
   * Returns the address length.
   */
  unsigned int getHLMACLength() const { return HLMAC_ADDRESS_SIZE * 8 / HLMAC_WIDTH -1; }

  /**
   * Returns the value of index.
   */
  unsigned char getIndexValue(unsigned int k) const;

  /**
   * Converts address to a hex string.
   */
  std::string str() const;

  /**
   * Converts address to 48 bits integer.
   */
  uint64 getInt() const { return address; }

  /**
   * Sets the address and returns true if the syntax of the string
   * is correct. (See setAddress() for the syntax.)
   */
  bool tryParse(const char *hexstr);

  /**
   * Sets the kth index of the address.
   */
  void setIndexValue(unsigned int k, unsigned char addrbyte);

  /**
   * Converts address value from hex string (getHLMACLength() hex digits, may also
   * contain spaces, hyphens and colons)
   */
  void setAddress(const char *hexstr);

  /* add new suffix to frame. note that this method cannot insert new core.
   * for insertion new core, we can directly use setIndexValue(0, core) or setCore()
   */
  void addNewId(unsigned char newPortId);

  void removeLastId();

  HLMACAddress getWithoutLastId();

  void setCore(unsigned char newCoreId);

  unsigned short int getHLMACHier();
  /**
   * Assignment.
   */
  HLMACAddress& operator=(const HLMACAddress& other) { address = other.address; return *this; }

  /**
   * Returns true if the two addresses are equal.
   */
  bool equals(const HLMACAddress& other) const { return address == other.address; }

  /**
   * Returns true if the two addresses are equal.
   */
  bool operator==(const HLMACAddress& other) const { return address == other.address; }

  /**
   * Returns true if the two addresses are not equal.
   */
  bool operator!=(const HLMACAddress& other) const { return address != other.address; }

  /**
   * Returns -1, 0 or 1 as result of comparison of 2 addresses.
   */
  int compareTo(const HLMACAddress& other) const;

  bool operator<(const HLMACAddress& other) const { return address < other.address; }

  bool operator<=(const HLMACAddress& other) const { return address <= other.address; }

  bool operator>(const HLMACAddress& other) const { return address > other.address; }

  bool operator>=(const HLMACAddress& other) const { return address >= other.address; }

  HLMACAddress getLongestCommonPrefix(const HLMACAddress& other);

  bool isPrefixOf(HLMACAddress other);

};
inline std::ostream& operator<<(std::ostream& os, const HLMACAddress& hlmac)
{
    return os << hlmac.str();
}

}

#endif /* IOTORII_SRC_LINKLAYER_COMMON_HLMACADDRESS_H_ */
