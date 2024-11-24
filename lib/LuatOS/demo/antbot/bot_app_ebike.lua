local bot_asset = require("bot_asset")

local bot_app_ebike = {}

--EBike data reporting format
local ebike_pub_data_fmt = [[
{
  "assetId": "%s",
  "vehStatus": %d,
  "mileage": %d,
  "batteryCap": %d,
  "soh": %d,
  "soc": %d,
  "voltage": %d,
  "temperature": %d,
  "fullChargeCycles": %d,
  "batteryStatus": %d,
  "dataTime": "%s",
  "coordinateSystem": %d,
  "longitude": %f,
  "latitude": %f,
  "altitude": %f,
  "source": %d
}
]]

--EBike asset type code
local asset_type_code_ebike = {
    EBICYCLE_TYPE_OTHERS  = 1011,           --other
    EBICYCLE_TYPE_BICYCLE = 1012,           --bicycle
    EBICYCLE_TYPE_PEDELEC = 1013            --moped
}

--EBike data structure
local function create_asset_data()
    return {
        vehStatus = 0,                      --vehicle status
        mileage = 0,                        --Single mileage, unit: meters
        batteryCap = 0,                     --Remaining battery capacity, unit: mAh
        soh = 0,                            --Battery health status, value range: [0-100]
        soc = 0,                            --Battery power, value range: [0-100]
        voltage = 0,                        --Battery voltage, unit: 0.01V
        temperature = 0,                    --Battery temperature, unit: 0.01℃
        fullChargeCycles = 0,               --Accumulated number of charge and discharge times
        batteryStatus = 0,                  --Battery status, 0-shelved, 1-charging, 2-discharging, 3-reserved
        dataTime = "",                      --Data collection time, "20211103173632" means 2021/11/03 17:36:32
        location = bot_asset.locationInfo() --Geolocation information
    }
end

local function user_asset_id_get()
    --Asset number, as the unique identifier of the asset, according to the actual modification, "BOT-TEST" is only used for testing
    --The user needs to fill in the actual situation
    return "BOT-TEST"
end


function bot_app_ebike.user_asset_config_get()
    local asset_config = bot_asset.configInfo()

    --/********************** The following information needs to be filled in by the user according to the actual situation********************** *******/

    local asset_id = user_asset_id_get()
    local asset_type_code = asset_type_code_ebike.EBICYCLE_TYPE_PEDELEC
    --Represents the asset model, the content is a custom string (and cannot contain the symbol "-")
    local asset_model = "WD215"

    --/********************** The above information needs to be filled in by the user according to the actual situation************************ *******/

    asset_config.id = asset_id
    asset_config.type = tostring(asset_type_code) .. "-" .. asset_model
    asset_config.adv = BOT_ASSET_DATA_VERSION

    return asset_config
end


function bot_app_ebike.user_asset_data_get()
    local ebike_data = create_asset_data()

    --/********************** The following information needs to be filled in by the user according to the actual situation********************** *******/

    --/*! ! ! Required fields, please refer to the project information table for details */
    --/* Business data needs to be filled in with actual operating data. Please do not delete the default fields. Int type is passed in by default: -1, string is passed in by default: "-" */
    ebike_data.vehStatus = 12               --/* Vehicle status */
    ebike_data.mileage = 5000               --/* Single mileage, unit: meters */
    ebike_data.batteryCap = 20000           --/* Remaining battery capacity, unit: mAh */
    ebike_data.soh = 90                     --/* Battery health status, value range: [0-100] */
    ebike_data.soc = 90                     --/* Battery power, value range: [0-100] */
    ebike_data.voltage = 3000               --/* Battery voltage, unit: 0.01V */
    ebike_data.temperature = 3000           --/* Battery temperature, unit: 0.01℃ */
    ebike_data.fullChargeCycles = 12        --/* Accumulated number of charge and discharge times */
    ebike_data.batteryStatus = 1            --/* Battery status, 0-shelved, 1-charging, 2-discharging, 3-reserved */
    ebike_data.dataTime = "20221027173632"  --/* Data collection time, "20211103173632" means 2021/11/03 17:36:32 */

    --/* Optional fields */
    --/* Geographical location information, if there is no geographical location information data, please do not delete the field, the default is passed in: -1 */
    ebike_data.location.coordinateSystem = 2    --/* Coordinate system, 1-WGS_84, 2-GCJ_02 */
    ebike_data.location.longitude = 121.5065267 --/* longitude */
    ebike_data.location.latitude = 31.2173088   --/* Latitude */
    ebike_data.location.altitude = 30.04        --/* Altitude */
    ebike_data.location.source = 0              --/* Positioning source */

    --/********************** The above information needs to be filled in by the user according to the actual situation************************ *******/

    --/* This is the information group package, users do not need to pay attention */
    local data = string.format(ebike_pub_data_fmt,
        user_asset_id_get(),
        ebike_data.vehStatus,
        ebike_data.mileage,
        ebike_data.batteryCap,
        ebike_data.soh,
        ebike_data.soc,
        ebike_data.voltage,
        ebike_data.temperature,
        ebike_data.fullChargeCycles,
        ebike_data.batteryStatus,
        ebike_data.dataTime,
        ebike_data.location.coordinateSystem,
        ebike_data.location.longitude,
        ebike_data.location.latitude,
        ebike_data.location.altitude,
        ebike_data.location.source
    )

    print("bot user data: " .. data, "len " .. #data)

    return data
end

return bot_app_ebike
