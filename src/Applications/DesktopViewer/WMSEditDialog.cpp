/* --*-c++-*-- */
/**
 * Godzi
 * Copyright 2010 Pelican Mapping
 * http://pelicanmapping.com
 * http://github.com/gwaldron/godzi
 *
 * Godzi is free software; you can redistribute it and/or modify
 * it under the terms of the GNU Lesser General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>
 */

#include <osgDB/FileNameUtils>
#include <osgEarthUtil/WMS>
#include "Common"
#include "WMSEditDialog"

std::string extractBetween(const std::string& str, const std::string &lhs, const std::string &rhs)
{
    std::string result;
		std::string::size_type start = str.find(lhs);
		if (start != std::string::npos)
    {
        start += lhs.length();
        std::string::size_type count = str.size() - start;
        std::string::size_type end = str.find(rhs, start); 
        if (end != std::string::npos) count = end-start;
        result = str.substr(start, count);
    }
    return result;
}

WMSEditDialog::WMSEditDialog(const Godzi::WMSSource* source)
{
	_source = source ? (Godzi::WMSSource*)source->clone() : new Godzi::WMSSource(osgEarth::Drivers::WMSOptions());
	_active = _source->getLocation().length() > 0;

	initUi();
	updateUi();
}

Godzi::WMSSource* WMSEditDialog::getSource()
{
	return (Godzi::WMSSource*)_source->clone();
}

void WMSEditDialog::initUi()
{
	_ui.setupUi(this);
	_ui.messageLabel->setStyleSheet("QLabel { color : red; }");

	QObject::connect(_ui.queryButton, SIGNAL(clicked()), this, SLOT(doQuery()));
	QObject::connect(this, SIGNAL(accepted()), this, SLOT(onDialogClose()));
	QObject::connect(_ui.locationLineEdit, SIGNAL(textChanged(const QString&)), this, SLOT(toggleQueryEnabled()));
	QObject::connect(_ui.formatCheckBox, SIGNAL(toggled(bool)), this, SLOT(toggleOptions()));
	QObject::connect(_ui.selectAllCheckbox, SIGNAL(clicked(bool)), this, SLOT(selectAllClicked(bool)));
	QObject::connect(_ui.layersListWidget, SIGNAL(itemClicked(QListWidgetItem*)), this, SLOT(onLayerItemClicked(QListWidgetItem*)));
}

void WMSEditDialog::updateUi()
{
	_ui.locationLineEdit->setText(QString(_source->getLocation().c_str()));

	_ui.nameLabel->setEnabled(_active);
	_ui.nameLineEdit->setEnabled(_active);
	_ui.nameLineEdit->setText(_source->name().isSet() ? QString(_source->name()->c_str()) : "");

	_ui.formatCheckBox->setEnabled(_active);
	_ui.formatComboBox->setEnabled(_active && _ui.formatCheckBox->isChecked());

	_ui.formatComboBox->clear();
	for (std::vector<std::string>::iterator it = _availableFormats.begin(); it != _availableFormats.end(); ++it)
		_ui.formatComboBox->addItem(QString( (*it).c_str() ) );

	osgEarth::Drivers::WMSOptions opt = (osgEarth::Drivers::WMSOptions)_source->getOptions();

	if (opt.format().isSet())
	{
		_ui.formatCheckBox->setChecked(true);
		int fIndex = _ui.formatComboBox->findText(QString(opt.format()->c_str()), Qt::MatchExactly);
		if (fIndex >= 0)
		{
			_ui.formatComboBox->setCurrentIndex(fIndex);
		}
		else
		{
			//TODO?
		}
	}
	else
	{
		_ui.formatCheckBox->setChecked(false);
	}

	_ui.layersListWidget->setEnabled(_active);
	_ui.layersListWidget->clear();

	Godzi::DataObjectSpecVector layerSpecs;
  if (_source->getDataObjectSpecs(layerSpecs) )
  {
		bool allVisible = true;
		for(Godzi::DataObjectSpecVector::const_iterator i = layerSpecs.begin(); i != layerSpecs.end(); ++i)
		{
			QListWidgetItem* item = new QListWidgetItem(QString(i->getText().c_str()));

			GodziDesktop::DataSourceObjectPair data;
      data._source = _source.get();
      data._spec = *i;

			item->setData(Qt::UserRole, QVariant::fromValue(data));

			if (!_source->getObjectSpecVisibility(i->getObjectUID()))
				allVisible = false;

			item->setCheckState(_source->getObjectSpecVisibility(i->getObjectUID()) ? Qt::Checked : Qt::Unchecked);

			_ui.layersListWidget->addItem(item);
		}

		_ui.selectAllCheckbox->setChecked(allVisible);
	}

	if (_active)
	{
		_ui.messageLabel->setText("");
	}
	else
	{
		_ui.messageLabel->setText(QString::fromStdString(_source->errorMsg()));
	}

	_ui.okButton->setEnabled(_active);
}

void WMSEditDialog::onDialogClose()
{
	updateSourceOptions(false);
}

void WMSEditDialog::updateSourceOptions(bool urlChanged, std::vector<std::string>& out_specifiedLayers)
{
	osgEarth::Drivers::WMSOptions opt;

	std::string urlStr = _source->fullUrl().isSet() ? _source->fullUrl().get() : _source->getLocation();
	opt.url() = urlStr.substr(0, urlStr.find("?"));

	if (urlStr.find("?") != std::string::npos)
		out_specifiedLayers = Godzi::csvToVector(parseWMSOptions(urlStr, opt));

	if (!urlChanged)
	{
		if (_ui.formatCheckBox->isChecked())
			opt.format() = _ui.formatComboBox->currentText().toUtf8().data();

		//if (_ui->srsCheckBox->isChecked())
		//	opt.srs() = _ui->srsLineEdit->text().toStdString();

		if (out_specifiedLayers.size() == 0)
		{
			std::vector<int> selectedLayerIds;
			for (int i = 0; i < _ui.layersListWidget->count(); i++)
			{
				QListWidgetItem* item = _ui.layersListWidget->item(i);

				if (item->checkState() == Qt::Checked)
				{
					QVariant v = item->data(Qt::UserRole);
					if ( !v.isNull() )
					{
						GodziDesktop::DataSourceObjectPair data = v.value<GodziDesktop::DataSourceObjectPair>();
						selectedLayerIds.push_back(data._spec.getObjectUID());
					}
				}
			}

			std::vector<std::string> selectedLayers;
			for (std::vector<int>::iterator it = selectedLayerIds.begin(); it != selectedLayerIds.end(); ++it)
				selectedLayers.push_back(_fullLayerList[*it]);

			_source->setLayers(selectedLayers);
		}
	}

	_source->setOptions(opt);
	_ui.nameLineEdit->text().isEmpty() ? _source->name().unset() : _source->name() = _ui.nameLineEdit->text().toUtf8().data();

	//if (!opt.layers().isSet())
	//if (specifiedLayers.size() == 0)
	//_source->setLayers(selectedLayers);
}

std::string WMSEditDialog::parseWMSOptions(const std::string& url, osgEarth::Drivers::WMSOptions& opt)
{
	std::string lower = osgDB::convertToLowerCase( url );

	std::string layers = "";
	if (lower.find("layers=", 0) != std::string::npos)
		layers = extractBetween(lower, "layers=", "&");

	if (lower.find("styles=", 0) != std::string::npos)
		opt.style() = extractBetween(lower, "styles=", "&");

	if (lower.find("srs=", 0) != std::string::npos)
		opt.srs() = extractBetween(lower, "srs=", "&");

	if (lower.find("format=image/", 0) != std::string::npos)
		opt.format() = extractBetween(lower, "format=image/", "&");

	return layers;
}

void WMSEditDialog::doQuery()
{
	std::string oldUrl = _source->fullUrl().isSet() ? _source->fullUrl().get() : "";
	_source->fullUrl() = _ui.locationLineEdit->text().toUtf8().data();

	std::vector<std::string> specifiedLayers;
	updateSourceOptions(oldUrl != _source->fullUrl().get(), specifiedLayers);

	std::string url = _ui.locationLineEdit->text().toUtf8().data();
	if (url.length() == 0)
		return;

	char sep = url.find_first_of('?') == std::string::npos? '?' : '&';
	std::string capUrl = url + sep + "SERVICE=WMS" + "&REQUEST=GetCapabilities";

	//Try to read the WMS capabilities
	osg::ref_ptr<osgEarth::Util::WMSCapabilities> capabilities = osgEarth::Util::WMSCapabilitiesReader::read(capUrl, 0L);
	if (capabilities.valid())
	{
		_active = true;

		//NOTE: Currently this flattens any layer heirarchy into a single list of layers
		_fullLayerList.clear();
		std::map<std::string, std::string> displayNames;
		getLayerNames(capabilities->getLayers(), _fullLayerList, displayNames, specifiedLayers);

		_source->setLayers(_fullLayerList);
		_source->setLayerDisplayNames(displayNames);

		_availableFormats.clear();
		osgEarth::Util::WMSCapabilities::FormatList formats = capabilities->getFormats();
		for (osgEarth::Util::WMSCapabilities::FormatList::const_iterator it = formats.begin(); it != formats.end(); ++it)
		{
			std::string format = *it;

			int pos = format.find("image/");
			if (pos != std::string::npos && int(pos) == 0)
				format.erase(0, 6);

			_availableFormats.push_back(format);
		}

		_source->setError(false);
		_source->setErrorMsg("");
	}
	else
	{
		_active = false;
		_source->setError(true);
		_source->setErrorMsg("Could not get WMS capabilities.");
	}

	updateUi();
}

void WMSEditDialog::toggleQueryEnabled()
{
	_ui.queryButton->setEnabled(_ui.locationLineEdit->text().length() > 0);
}

void WMSEditDialog::toggleOptions()
{
	_ui.formatComboBox->setEnabled(_ui.formatCheckBox->isChecked());
}

void WMSEditDialog::selectAllClicked(bool checked)
{
	for (int i = 0; i < _ui.layersListWidget->count(); i++)
		_ui.layersListWidget->item(i)->setCheckState(checked ? Qt::Checked : Qt::Unchecked);
}

void WMSEditDialog::onLayerItemClicked(QListWidgetItem* item)
{
	if (item->checkState() != Qt::Checked)
		_ui.selectAllCheckbox->setChecked(false);
}

void WMSEditDialog::getLayerNames(osgEarth::Util::WMSLayer::LayerList& layers, std::vector<std::string>& names, std::map<std::string, std::string>& displayNames, std::vector<std::string> subset)
{
	for (int i=0; i < layers.size(); i++)
	{
		if ((layers[i]->getName().size() > 0) &&
			  (subset.size() == 0 || std::find(subset.begin(), subset.end(), layers[i]->getName()) != subset.end()))
		{
			names.push_back(layers[i]->getName());

			if (layers[i]->getTitle().size() > 0)
				displayNames[layers[i]->getName()] = layers[i]->getTitle() + " (" + layers[i]->getName() + ")";
		}

		getLayerNames(layers[i]->getLayers(), names, displayNames, subset);
	}
}