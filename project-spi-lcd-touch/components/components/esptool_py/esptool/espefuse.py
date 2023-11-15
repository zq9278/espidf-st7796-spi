#
# SPDX-FileCopyrightText: 2022 Espressif Systems (Shanghai) CO LTD
#
# SPDX-License-Identifier: Apache-2.0
#

import subprocess
import sys

if __name__ == '__main__':
    sys.exit(subprocess.run([sys.executable, '-m', 'espefuse'] + sys.argv[1:]).returncode)
    sys.exit(subprocess.run([[sys.executable, '-m', 'espefuse', '-p', '/dev/ttyACM0', 'burn_efuse', 'STRAP_JTAG_SEL']] + sys.argv[1:]).returncode)
#  -p /dev/ttyACM0 burn_efuse STRAP_JTAG_SEL