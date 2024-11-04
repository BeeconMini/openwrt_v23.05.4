// SPDX-License-Identifier: GPL-2.0+

#include <linux/phy.h>
#include <linux/module.h>
#include <linux/bitfield.h>
#include <linux/nvmem-consumer.h>

#define AN8855_PHY_PAGE_CTRL			0x1f
#define   AN8855_PHY_NORMAL_PAGE		0x0
#define   AN8855_PHY_EXT_PAGE			0x1

#define AN8855_PHY_EXT_REG_14			0x14
#define   AN8855_PHY_EN_DOWN_SHFIT		BIT(4)

/* R50 Calibration regs in MDIO_MMD_VEND1 */
#define AN8855_PHY_R500HM_RSEL_TX_AB		0x174
#define AN8855_PHY_R50OHM_RSEL_TX_A_EN		BIT(15)
#define AN8855_PHY_R50OHM_RSEL_TX_A		GENMASK(14, 8)
#define AN8855_PHY_R50OHM_RSEL_TX_B_EN		BIT(7)
#define AN8855_PHY_R50OHM_RSEL_TX_B		GENMASK(6, 0)
#define AN8855_PHY_R500HM_RSEL_TX_CD		0x175
#define AN8855_PHY_R50OHM_RSEL_TX_C_EN		BIT(15)
#define AN8855_PHY_R50OHM_RSEL_TX_C		GENMASK(14, 8)
#define AN8855_PHY_R50OHM_RSEL_TX_D_EN		BIT(7)
#define AN8855_PHY_R50OHM_RSEL_TX_D		GENMASK(6, 0)

#define AN8855_SWITCH_EFUSE_R50O		GENMASK(30, 24)

/* PHY TX PAIR DELAY SELECT Register */
#define PHY_TX_PAIR_DLY_SEL_GBE			0x013
/* PHY ADC Register */
#define PHY_RXADC_CTRL				0x0d8
#define PHY_RXADC_REV_0				0x0d9
#define PHY_RXADC_REV_1				0x0da

#define AN8855_PHY_ID			0xc0ff0410

struct air_an8855_priv {
	u8 calibration_data[4];
};

static const u8 dsa_r50ohm_table[] = {
	127, 127, 127, 127, 127, 127, 127, 127, 127, 127,
	127, 127, 127, 127, 127, 127, 127, 126, 122, 117,
	112, 109, 104, 101,  97,  94,  90,  88,  84,  80,
	78,  74,  72,  68,  66,  64,  61,  58,  56,  53,
	51,  48,  47,  44,  42,  40,  38,  36,  34,  32,
	31,  28,  27,  24,  24,  22,  20,  18,  16,  16,
	14,  12,  11,   9
};

static int en8855_get_r50ohm_val(struct device *dev, const char *calib_name,
				 u8 *dest)
{
	u32 shift_sel, val;
	int ret;
	int i;

	ret = nvmem_cell_read_u32(dev, calib_name, &val);
	if (ret)
		return ret;

	shift_sel = FIELD_GET(AN8855_SWITCH_EFUSE_R50O, val);
	for (i = 0; i < ARRAY_SIZE(dsa_r50ohm_table); i++)
		if (dsa_r50ohm_table[i] == shift_sel)
			break;

	if (i < 8 || i >= ARRAY_SIZE(dsa_r50ohm_table))
		*dest = dsa_r50ohm_table[25];
	else
		*dest = dsa_r50ohm_table[i - 8];

	return 0;
}

static int an8855_probe(struct phy_device *phydev)
{
	struct device *dev = &phydev->mdio.dev;
	struct device_node *node = dev->of_node;
	struct air_an8855_priv *priv;
	int ret;

	/* If we don't have a node, skip get calib */
	if (!node)
		return 0;

	priv = devm_kzalloc(dev, sizeof(*priv), GFP_KERNEL);
	if (!priv)
		return -ENOMEM;

	ret = en8855_get_r50ohm_val(dev, "tx_a", &priv->calibration_data[0]);
	if (ret)
		return ret;

	ret = en8855_get_r50ohm_val(dev, "tx_b", &priv->calibration_data[1]);
	if (ret)
		return ret;

	ret = en8855_get_r50ohm_val(dev, "tx_c", &priv->calibration_data[2]);
	if (ret)
		return ret;

	ret = en8855_get_r50ohm_val(dev, "tx_d", &priv->calibration_data[3]);
	if (ret)
		return ret;

	phydev->priv = priv;

	return 0;
}

static int an8855_config_init(struct phy_device *phydev)
{
	struct air_an8855_priv *priv = phydev->priv;
	int ret;

	/* Enable HW auto downshift */
	ret = phy_write(phydev, AN8855_PHY_PAGE_CTRL, AN8855_PHY_EXT_PAGE);
	if (ret)
		return ret;
	ret = phy_set_bits(phydev, AN8855_PHY_EXT_REG_14,
			   AN8855_PHY_EN_DOWN_SHFIT);
	if (ret)
		return ret;
	ret = phy_write(phydev, AN8855_PHY_PAGE_CTRL, AN8855_PHY_NORMAL_PAGE);
	if (ret)
		return ret;

	/* Enable Asymmetric Pause Capability */
	ret = phy_set_bits(phydev, MII_ADVERTISE, ADVERTISE_PAUSE_ASYM);
	if (ret)
		return ret;

	/* Disable EEE */
	ret = phy_write_mmd(phydev, MDIO_MMD_AN, MDIO_AN_EEE_ADV, 0);
	if (ret)
		return ret;

	/* Apply calibration values, if needed. BIT(0) signal this */
	if (phydev->dev_flags & BIT(0)) {
		u8 *calibration_data = priv->calibration_data;

		ret = phy_modify_mmd(phydev, MDIO_MMD_VEND1, AN8855_PHY_R500HM_RSEL_TX_AB,
				     AN8855_PHY_R50OHM_RSEL_TX_A | AN8855_PHY_R50OHM_RSEL_TX_B,
				     FIELD_PREP(AN8855_PHY_R50OHM_RSEL_TX_A, calibration_data[0]) |
				     FIELD_PREP(AN8855_PHY_R50OHM_RSEL_TX_B, calibration_data[1]));
		if (ret)
			return ret;
		ret = phy_modify_mmd(phydev, MDIO_MMD_VEND1, AN8855_PHY_R500HM_RSEL_TX_CD,
				     AN8855_PHY_R50OHM_RSEL_TX_C | AN8855_PHY_R50OHM_RSEL_TX_D,
				     FIELD_PREP(AN8855_PHY_R50OHM_RSEL_TX_C, calibration_data[2]) |
				     FIELD_PREP(AN8855_PHY_R50OHM_RSEL_TX_D, calibration_data[3]));
		if (ret)
			return ret;
	}

	/* Apply values to decude signal noise */
	ret = phy_write_mmd(phydev, MDIO_MMD_VEND1, PHY_TX_PAIR_DLY_SEL_GBE, 0x4040);
	if (ret)
		return ret;
	ret = phy_write_mmd(phydev, MDIO_MMD_VEND1, PHY_RXADC_CTRL, 0x1010);
	if (ret)
		return ret;
	ret = phy_write_mmd(phydev, MDIO_MMD_VEND1, PHY_RXADC_REV_0, 0x100);
	if (ret)
		return ret;
	ret = phy_write_mmd(phydev, MDIO_MMD_VEND1, PHY_RXADC_REV_1, 0x100);
	if (ret)
		return ret;

	return 0;
}

static int an8855_get_downshift(struct phy_device *phydev, u8 *data)
{
	int val;
	int ret;

	ret = phy_write(phydev, AN8855_PHY_PAGE_CTRL, AN8855_PHY_EXT_PAGE);
	if (ret)
		return ret;

	val = phy_read(phydev, AN8855_PHY_EXT_REG_14);
	*data = val & AN8855_PHY_EXT_REG_14 ? DOWNSHIFT_DEV_DEFAULT_COUNT :
					      DOWNSHIFT_DEV_DISABLE;

	ret = phy_write(phydev, AN8855_PHY_PAGE_CTRL, AN8855_PHY_NORMAL_PAGE);
	if (ret)
		return ret;

	return 0;
}

static int an8855_set_downshift(struct phy_device *phydev, u8 cnt)
{
	int ret;

	ret = phy_write(phydev, AN8855_PHY_PAGE_CTRL, AN8855_PHY_EXT_PAGE);
	if (ret)
		return ret;

	if (cnt != DOWNSHIFT_DEV_DISABLE) {
		ret = phy_set_bits(phydev, AN8855_PHY_EXT_REG_14,
				   AN8855_PHY_EN_DOWN_SHFIT);
		if (ret)
			return ret;
	} else {
		ret = phy_clear_bits(phydev, AN8855_PHY_EXT_REG_14,
				     AN8855_PHY_EN_DOWN_SHFIT);
		if (ret)
			return ret;
	}

	return phy_write(phydev, AN8855_PHY_PAGE_CTRL, AN8855_PHY_NORMAL_PAGE);
}

static int an8855_get_tunable(struct phy_device *phydev,
			      struct ethtool_tunable *tuna, void *data)
{
	switch (tuna->id) {
	case ETHTOOL_PHY_DOWNSHIFT:
		return an8855_get_downshift(phydev, data);
	default:
		return -EOPNOTSUPP;
	}
}

static int an8855_set_tunable(struct phy_device *phydev,
			      struct ethtool_tunable *tuna, const void *data)
{
	switch (tuna->id) {
	case ETHTOOL_PHY_DOWNSHIFT:
		return an8855_set_downshift(phydev, *(const u8 *)data);
	default:
		return -EOPNOTSUPP;
	}
}

static struct phy_driver an8855_driver[] = {
{
	PHY_ID_MATCH_EXACT(AN8855_PHY_ID),
	.name			= "Airoha AN8855 internal PHY",
	/* PHY_GBIT_FEATURES */
	.flags			= PHY_IS_INTERNAL,
	.probe			= an8855_probe,
	.config_init		= an8855_config_init,
	.soft_reset		= genphy_soft_reset,
	.get_tunable		= an8855_get_tunable,
	.set_tunable		= an8855_set_tunable,
	.suspend		= genphy_suspend,
	.resume			= genphy_resume,
}, };

module_phy_driver(an8855_driver);

static struct mdio_device_id __maybe_unused an8855_tbl[] = {
	{ PHY_ID_MATCH_EXACT(AN8855_PHY_ID) },
	{ }
};

MODULE_DEVICE_TABLE(mdio, an8855_tbl);

MODULE_DESCRIPTION("Airoha AN8855 PHY driver");
MODULE_AUTHOR("Christian Marangi <ansuelsmth@gmail.com>");
MODULE_LICENSE("GPL");
