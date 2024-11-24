local bot_asset = {}
--The configuration token used for testing. For mass production token, please contact Ant students to obtain it.
BOT_CONFIG_TOKEN = "iBot-rawData&RUBDPmYzoj2ftkNLEUKBbLjYUNoVHtFZUj0fKTgF8fpkXZbQcbOaNkFuL69rPZ3M0g=="

--The asset data field version number (assetDataVersion) is defined by Ant. The default value is "ADV1.0" and does not need to be modified.
BOT_ASSET_DATA_VERSION = "ADV1.0"
--Maximum length of asset number
BOT_ASSET_ID_MAX_SIZE = 64
--Asset type maximum length
BOT_ASSET_TYPE_MAX_SIZE = 64
--Maximum length of asset data version
BOT_ASSET_ADV_MAX_SIZE = 16
--Maximum length of business data packet
BOT_USER_DATA_STRING_MAX_SIZE = 1024
--Maximum number of retries when registration fails
BOT_REG_RETRY_COUNT_MAX = 3

--Common asset allocation data structure
function bot_asset.configInfo()
    return {
        id = "",
        type = "",
        adv = ""
    }
end

--Generic geolocation data structure
function bot_asset.locationInfo()
    return {
        coordinateSystem = 0,
        longitude = 0.0,
        latitude = 0.0,
        altitude = 0.0,
        source = 0
    }
end

return bot_asset
