/**
 *
 * config asio as standalone(no boost dependency)
 *
 * @author  :   yandaren1220@126.com
 * @date    :   2017-08-25
 */

#ifndef __ydk_utility_asio_base_asio_standalone_config_hpp__
#define __ydk_utility_asio_base_asio_standalone_config_hpp__

#ifndef ASIO_STANDALONE
#define ASIO_STANDALONE
#endif

#ifndef ASIO_HAS_STD_CHRONO
#define ASIO_HAS_STD_CHRONO 1
#endif

#endif