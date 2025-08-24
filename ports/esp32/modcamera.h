/*
 * The MIT License (MIT)
 *
 * Copyright [2025] freshev
 * 
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#ifndef MICROPY_INCLUDED_ESP32_MODCAMERA_H
#define MICROPY_INCLUDED_ESP32_MODCAMERA_H

#define TAG "camera"

#define XCLK_FREQ_10MHz    10000000
#define XCLK_FREQ_20MHz    20000000

//White Balance
#define WB_NONE     0
#define WB_SUNNY    1
#define WB_CLOUDY   2
#define WB_OFFICE   3
#define WB_HOME     4

//Special Effect
#define EFFECT_NONE    0
#define EFFECT_NEG     1
#define EFFECT_BW      2
#define EFFECT_RED     3
#define EFFECT_GREEN   4
#define EFFECT_BLUE    5
#define EFFECT_RETRO   6

#endif
