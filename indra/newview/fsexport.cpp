/**
 * @file fsexport.cpp
 * @brief export selected objects to an file using LLSD.
 *
 * $LicenseInfo:firstyear=2013&license=viewerlgpl$
 * Phoenix Firestorm Viewer Source Code
 * Copyright (c) 2013 Techwolf Lupindo
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation;
 * version 2.1 of the License only.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA
 * 
 * The Phoenix Firestorm Project, Inc., 1831 Oakwood Drive, Fairmont, Minnesota 56031-3225 USA
 * http://www.firestormviewer.org
 * $/LicenseInfo$
 */

#include "llviewerprecompiledheaders.h"

#include "fsexport.h"

#include "llagent.h"
#include "llagentconstants.h"
#include "llagentdata.h"
#include "llappviewer.h"
#include "llavatarnamecache.h"
#include "llbufferstream.h"
#include "llcallbacklist.h"
#include "lldatapacker.h"
#include "lldir.h"
#include "llfilepicker.h"
#include "llimagej2c.h"
#include "llinventoryfunctions.h"
#include "llmeshrepository.h"
#include "llmultigesture.h"
#include "llnotecard.h"
#include "llnotificationsutil.h"
#include "llsdserialize.h"
#include "llsdutil_math.h"
#include "llsdutil.h"
#include "lltrans.h"
#include "llversioninfo.h"
#include "llvfile.h"
#include "llviewerinventory.h"
#include "llviewernetwork.h"
#include "llviewerregion.h"
#include "llviewertexturelist.h"
#include "llvovolume.h"
#include "llviewerpartsource.h"
#include "llworld.h"
#include "fscommon.h"
#include "llfloaterreg.h"
#include <boost/algorithm/string_regex.hpp>

const F32 MAX_TEXTURE_WAIT_TIME = 30.0f;
const F32 MAX_INVENTORY_WAIT_TIME = 30.0f;
const F32 MAX_ASSET_WAIT_TIME = 60.0f;

static void updateProgress(const std::string message);

// static
void FSExport::onIdle(void* user_data)
{
	FSExport* self = (FSExport*)user_data;
	self->onIdle();
}

void FSExport::onIdle()
{
	switch(mExportState)
	{
	case IDLE:
		break;
	case INVENTORY_DOWNLOAD:
		if (gDisconnected)
		{
			return;
		}
		
		if (mInventoryRequests.empty())
		{
			mLastRequest = mAssetRequests.size();
			mWaitTimer.start();
			mExportState = ASSET_DOWNLOAD;
		}
		else if (mLastRequest != mInventoryRequests.size())
		{
			mWaitTimer.start();
			mLastRequest = mInventoryRequests.size();
		}
		else if (mWaitTimer.getElapsedTimeF32() > MAX_INVENTORY_WAIT_TIME)
		{
			mWaitTimer.start();
			for (uuid_vec_t::const_iterator iter = mInventoryRequests.begin(); iter != mInventoryRequests.end(); ++iter)
			{
				LLViewerObject* object = gObjectList.findObject((*iter));
				object->dirtyInventory();
				object->requestInventory();
				
				LLStringUtil::format_map_t args;
				args["ITEM"] = (*iter).asString();
				updateProgress(formatString(LLTrans::getString("export_rerequesting_inventory"), args));
				LL_DEBUGS("export") << "re-requested inventory of " << (*iter).asString() << LL_ENDL;
			}
		}
		break;
	case ASSET_DOWNLOAD:
		if (gDisconnected)
		{
			return;
		}
		
		if (mAssetRequests.empty())
		{
			mLastRequest = mRequestedTexture.size();
			mWaitTimer.start();
			mExportState = TEXTURE_DOWNLOAD;
		}
		else if (mLastRequest != mAssetRequests.size())
		{
			mWaitTimer.start();
			mLastRequest = mAssetRequests.size();
		}
		else if (mWaitTimer.getElapsedTimeF32() > MAX_ASSET_WAIT_TIME)
		{
			//abort for now
			LL_DEBUGS("export") << "Asset timeout with " << (S32)mAssetRequests.size() << " requests left." << LL_ENDL;
			for (uuid_vec_t::iterator iter = mAssetRequests.begin(); iter != mAssetRequests.end(); ++iter)
			{
				LL_DEBUGS("export") << "Asset: " << (*iter).asString() << LL_ENDL;
			}
			mAssetRequests.clear();
		}
		break;
	case TEXTURE_DOWNLOAD:
		if (gDisconnected)
		{
			return;
		}

		if(mRequestedTexture.empty())
		{
			mExportState = IDLE;
			if (!gIdleCallbacks.deleteFunction(onIdle, this))
			{
				LL_WARNS("export") << "Failed to delete idle callback" << LL_ENDL;
			}
			mWaitTimer.stop();

			llofstream file;
			file.open(mFileName, std::ios_base::out | std::ios_base::binary);
			std::string zip_data = zip_llsd(mFile);
			file.write(zip_data.data(), zip_data.size());
			file.close();
			LL_DEBUGS("export") << "Export finished and written to " << mFileName << LL_ENDL;
			
			updateProgress(LLTrans::getString("export_finished"));
			LLSD args;
			args["FILENAME"] = mFileName;
			LLNotificationsUtil::add("ExportFinished", args);
		}
		else if (mLastRequest != mRequestedTexture.size())
		{
			mWaitTimer.start();
			mLastRequest = mRequestedTexture.size();
		}
		else if (mWaitTimer.getElapsedTimeF32() > MAX_TEXTURE_WAIT_TIME)
		{
			mWaitTimer.start();
			for (std::map<LLUUID, FSAssetResourceData>::iterator iter = mRequestedTexture.begin(); iter != mRequestedTexture.end(); ++iter)
			{
				LLUUID texture_id = iter->first;
				LLViewerFetchedTexture* image = LLViewerTextureManager::getFetchedTexture(texture_id, FTT_DEFAULT, MIPMAP_TRUE);
				image->setBoostLevel(LLViewerTexture::BOOST_MAX_LEVEL);
				image->forceToSaveRawImage(0);
				image->setLoadedCallback(FSExport::onImageLoaded, 0, TRUE, FALSE, this, &mCallbackTextureList);
				
				LLStringUtil::format_map_t args;
				args["ITEM"] = texture_id.asString();
				updateProgress(formatString(LLTrans::getString("export_rerequesting_texture"), args));
				LL_DEBUGS("export") << "re-requested texture " << texture_id.asString() << LL_ENDL;
			}
		}
		break;
	default:
		break;
	}
}

void FSExport::exportSelection()
{
	LLObjectSelectionHandle selection = LLSelectMgr::instance().getSelection();
	if (!selection)
	{
		LL_WARNS("export") << "Nothing selected; Bailing!" << LL_ENDL;
		return;
	}
	LLObjectSelection::valid_root_iterator iter = selection->valid_root_begin();
	LLSelectNode* node = *iter;
	if (!node)
	{
		LL_WARNS("export") << "No node selected; Bailing!" << LL_ENDL;
		return;
	}

	LLFilePicker& file_picker = LLFilePicker::instance();
	if(!file_picker.getSaveFile(LLFilePicker::FFSAVE_EXPORT, LLDir::getScrubbedFileName(node->mName + ".oxp")))
	{
		llinfos << "User closed the filepicker, aborting export!" << llendl;
		return;
	}
	mFileName = file_picker.getFirstFile();
	mFilePath = gDirUtilp->getDirName(mFileName);
	
	mFile.clear();
	mRequestedTexture.clear();

	mExported = false;
	mAborted = false;
	mInventoryRequests.clear();
	mAssetRequests.clear();
	mTextureChecked.clear();

	mFile["format_version"] = 1;
	mFile["client"] = LLAppViewer::instance()->getSecondLifeTitle() + LLVersionInfo::getChannel();
	mFile["client_version"] = LLVersionInfo::getVersion();
	mFile["grid"] = LLGridManager::getInstance()->getGridLabel();
	LLFloaterReg::showInstance("fs_export");
	updateProgress(LLTrans::getString("export_started"));

	for ( ; iter != selection->valid_root_end(); ++iter)
	{
		mFile["linkset"].append(getLinkSet((*iter)));
	}

	if (mExported && !mAborted)
	{
		mWaitTimer.start();
		mLastRequest = mInventoryRequests.size();
		mExportState = INVENTORY_DOWNLOAD;
		gIdleCallbacks.addFunction(onIdle, this);
	}
	else
	{
		updateProgress(LLTrans::getString("export_nothing_exported"));
		LL_WARNS("export") << "Nothing was exported. File not created." << LL_ENDL;
	}
}

LLSD FSExport::getLinkSet(LLSelectNode* node)
{
	LLSD linkset;
	LLViewerObject* object = node->getObject();
	LLUUID object_id = object->getID();

	// root prim
	linkset.append(object_id);
	addPrim(object, true);

	// child prims
	LLViewerObject::const_child_list_t& child_list = object->getChildren();
	for (LLViewerObject::child_list_t::const_iterator iter = child_list.begin();
		 iter != child_list.end(); ++iter)
	{
		LLViewerObject* child = *iter;
		linkset.append(child->getID());
		addPrim(child, false);
	}

	return linkset;
}

void FSExport::addPrim(LLViewerObject* object, bool root)
{
	LLSD prim;
	LLUUID object_id = object->getID();
	bool default_prim = true;

	struct f : public LLSelectedNodeFunctor
	{
		LLUUID mID;
		f(const LLUUID& id) : mID(id) {}
		virtual bool apply(LLSelectNode* node)
		{
			return (node->getObject() && node->getObject()->mID == mID);
		}
	} func(object_id);
	LLSelectNode* node = LLSelectMgr::getInstance()->getSelection()->getFirstNode(&func);
	if (node)
	{
		if ((LLGridManager::getInstance()->isInSecondLife())
			&& object->permYouOwner()
			&& (gAgentID == node->mPermissions->getCreator() || megaPrimCheck(node->mPermissions->getCreator(), object)))
		{
			default_prim = false;
		}
#if OPENSIM
		if (LLGridManager::getInstance()->isInOpenSim())
		{
			LLViewerRegion* region = gAgent.getRegion();
			if (region && region->regionSupportsExport() == LLViewerRegion::EXPORT_ALLOWED)
			{
				default_prim = !node->mPermissions->allowExportBy(gAgent.getID());
			}
			else if (region && region->regionSupportsExport() == LLViewerRegion::EXPORT_DENIED)
			{
				// Only your own creations if this is explicitly set
				default_prim = (!(object->permYouOwner()
								  && gAgentID == node->mPermissions->getCreator()));
			}
			/// TODO: Once enough grids adopt a version supporting the exports cap, get consensus
			/// on whether we should allow full perm exports anymore.
			else	// LLViewerRegion::EXPORT_UNDEFINED
			{
				default_prim = (!(object->permYouOwner()
								  && object->permModify()
								  && object->permCopy()
								  && object->permTransfer()));
			}
		}
#endif
	}
	else
	{
		LL_WARNS("export") << "LLSelect node for " << object_id.asString() << " not found. Using default prim instead." << LL_ENDL;
		LLStringUtil::format_map_t args;
		args["OBJECT"] = object_id.asString();
		updateProgress(formatString(LLTrans::getString("export_node_not_found"), args));
		default_prim = true;
	}

	if (root)
	{
		if (object->isAttachment())
		{
			prim["attachment_point"] = ATTACHMENT_ID_FROM_STATE(object->getState());
		}
	}
	else
	{
		LLViewerObject* parent_object = (LLViewerObject*)object->getParent();
		prim["parent"] = parent_object->getID();
	}
	prim["position"] = object->getPosition().getValue();
	prim["scale"] = object->getScale().getValue();
	prim["rotation"] = ll_sd_from_quaternion(object->getRotation());

	if (default_prim)
	{
		LLStringUtil::format_map_t args;
		args["OBJECT"] = object_id.asString();
		updateProgress(formatString(LLTrans::getString("export_failed_export_check"), args));
		
		LL_DEBUGS("export") << object_id.asString() << " failed export check. Using default prim" << LL_ENDL;
		prim["flags"] = ll_sd_from_U32((U32)0);
		prim["volume"]["path"] = LLPathParams().asLLSD();
		prim["volume"]["profile"] = LLProfileParams().asLLSD();
		prim["material"] = (S32)LL_MCODE_WOOD;
	}
	else
	{
		mExported = true;
		prim["flags"] = ll_sd_from_U32(object->getFlags());
		prim["volume"]["path"] = object->getVolume()->getParams().getPathParams().asLLSD();
		prim["volume"]["profile"] = object->getVolume()->getParams().getProfileParams().asLLSD();
		prim["material"] = (S32)object->getMaterial();
		if (object->getClickAction() != 0)
		{
			prim["clickaction"] = (S32)object->getClickAction();
		}

		LLVOVolume *volobjp = NULL;
		if (object->getPCode() == LL_PCODE_VOLUME)
		{
			volobjp = (LLVOVolume *)object;
		}
		if(volobjp)
		{
			if(volobjp->isSculpted())
			{
				const LLSculptParams *sculpt_params = (const LLSculptParams *)object->getParameterEntry(LLNetworkData::PARAMS_SCULPT);
				if (sculpt_params)
				{
					if(volobjp->isMesh())
					{
						if (!mAborted)
						{
							updateProgress(LLTrans::getString("export_fail_no_mesh"));
							mAborted = true;
						}
						return;
					}
					else
					{
						if (exportTexture(sculpt_params->getSculptTexture()))
						{
							prim["sculpt"] = sculpt_params->asLLSD();
						}
						else
						{
							LLSculptParams default_sculpt;
							prim["sculpt"] = default_sculpt.asLLSD();
						}
					}
				}
			}

			if(volobjp->isFlexible())
			{
				const LLFlexibleObjectData *flexible_param_block = (const LLFlexibleObjectData *)object->getParameterEntry(LLNetworkData::PARAMS_FLEXIBLE);
				if (flexible_param_block)
				{
					prim["flexible"] = flexible_param_block->asLLSD();
				}
			}

			if (volobjp->getIsLight())
			{
				const LLLightParams *light_param_block = (const LLLightParams *)object->getParameterEntry(LLNetworkData::PARAMS_LIGHT);
				if (light_param_block)
				{
					prim["light"] = light_param_block->asLLSD();
				}
			}

			if (volobjp->hasLightTexture())
			{
				const LLLightImageParams* light_image_param_block = (const LLLightImageParams*)object->getParameterEntry(LLNetworkData::PARAMS_LIGHT_IMAGE);
				if (light_image_param_block)
				{
					prim["light_texture"] = light_image_param_block->asLLSD();
				}
			}
			
		}

		if(object->isParticleSource())
		{
			LLViewerPartSourceScript* partSourceScript = object->getPartSourceScript();
			prim["particle"] = partSourceScript->mPartSysData.asLLSD();
			if (!exportTexture(partSourceScript->mPartSysData.mPartImageID))
			{
				prim["particle"]["PartImageID"] = LLUUID::null.asString();
			}
		}

		U8 texture_count = object->getNumTEs();
		for(U8 i = 0; i < texture_count; ++i)
		{
			LLTextureEntry *checkTE = object->getTE(i);
			LL_DEBUGS("export") << "Checking texture number " << (S32)i
				<< ", ID " << checkTE->getID() << LL_ENDL;
			if (defaultTextureCheck(checkTE->getID()))	// <FS:CR> Check against default textures
			{
				LL_DEBUGS("export") << "...is a default texture." << LL_ENDL;
				prim["texture"].append(checkTE->asLLSD());
			}
			else if (exportTexture(checkTE->getID()))
			{
				LL_DEBUGS("export") << "...export check passed." << LL_ENDL;
				prim["texture"].append(checkTE->asLLSD());
			}
			else
			{
				LL_DEBUGS("export") << "...export check failed." << LL_ENDL;
				LLTextureEntry te(LL_DEFAULT_WOOD_UUID); // TODO: use user option of default texture.
				prim["texture"].append(te.asLLSD());
			}
		}

		if (!object->getPhysicsShapeUnknown())
		{
			prim["ExtraPhysics"]["PhysicsShapeType"] = (S32)object->getPhysicsShapeType();
			prim["ExtraPhysics"]["Density"] = (F64)object->getPhysicsDensity();
			prim["ExtraPhysics"]["Friction"] = (F64)object->getPhysicsFriction();
			prim["ExtraPhysics"]["Restitution"] = (F64)object->getPhysicsRestitution();
			prim["ExtraPhysics"]["GravityMultiplier"] = (F64)object->getPhysicsGravity();
		}

		prim["name"] = node->mName;
		prim["description"] = node->mDescription;
		prim["creation_date"] = ll_sd_from_U64(node->mCreationDate);

		LLAvatarName avatar_name;
		LLUUID creator_id = node->mPermissions->getCreator();
		if (creator_id.notNull())
		{
			prim["creator_id"] = creator_id;
			if (LLAvatarNameCache::get(creator_id, &avatar_name))
			{
				prim["creator_name"] = avatar_name.asLLSD();
			}
		}
		LLUUID owner_id = node->mPermissions->getOwner();
		if (owner_id.notNull())
		{
			prim["owner_id"] = owner_id;
			if (LLAvatarNameCache::get(owner_id, &avatar_name))
			{
				prim["owner_name"] = avatar_name.asLLSD();
			}
		}
		LLUUID group_id = node->mPermissions->getGroup();
		if (group_id.notNull())
		{
			prim["group_id"] = group_id;
			if (LLAvatarNameCache::get(group_id, &avatar_name))
			{
				prim["group_name"] = avatar_name.asLLSD();
			}
		}
		LLUUID last_owner_id = node->mPermissions->getLastOwner();
		if (last_owner_id.notNull())
		{
			prim["last_owner_id"] = last_owner_id;
			if (LLAvatarNameCache::get(last_owner_id, &avatar_name))
			{
				prim["last_owner_name"] = avatar_name.asLLSD();
			}
		}
		prim["base_mask"] = ll_sd_from_U32(node->mPermissions->getMaskBase());
		prim["owner_mask"] = ll_sd_from_U32(node->mPermissions->getMaskOwner());
		prim["group_mask"] = ll_sd_from_U32(node->mPermissions->getMaskGroup());
		prim["everyone_mask"] = ll_sd_from_U32(node->mPermissions->getMaskEveryone());
		prim["next_owner_mask"] = ll_sd_from_U32(node->mPermissions->getMaskNextOwner());
		
		prim["sale_info"] = node->mSaleInfo.asLLSD();
		prim["touch_name"] = node->mTouchName;
		prim["sit_name"] = node->mSitName;

		mInventoryRequests.push_back(object_id);
		object->registerInventoryListener(this, NULL);
		object->dirtyInventory();
		object->requestInventory();
	}

	mFile["prim"][object_id.asString()] = prim;
}

bool FSExport::exportTexture(const LLUUID& texture_id)
{
	if(texture_id.isNull())
	{
		updateProgress(LLTrans::getString("export_failed_null_texture"));
		LL_WARNS("export") << "Attempted to export NULL texture." << LL_ENDL;
		return false;
	}
	
	if (mTextureChecked.count(texture_id) != 0)
	{
		return mTextureChecked[texture_id];
	}
	
	if (gAssetStorage->mStaticVFS->getExists(texture_id, LLAssetType::AT_TEXTURE))
	{
		LL_DEBUGS("export") << "Texture " << texture_id.asString() << " is local static." << LL_ENDL;
		// no need to save the texture data as the viewer allready has it in a local file.
		mTextureChecked[texture_id] = true;
		return true;
	}
	
	//TODO: check for local file static texture. The above will only get the static texture in the static db, not indevitial texture files.

	LLViewerFetchedTexture* imagep = LLViewerTextureManager::getFetchedTexture(texture_id);
	bool texture_export = false;
	std::string name;
	std::string description;
	
	if (LLGridManager::getInstance()->isInSecondLife())
	{
		if (imagep->mComment.find("a") != imagep->mComment.end())
		{
			if (LLUUID(imagep->mComment["a"]) == gAgentID)
			{
				texture_export = true;
				LL_DEBUGS("export") << texture_id <<  " pass texture export comment check." << LL_ENDL;
			}
		}
	}

	if (texture_export)
	{
		assetCheck(texture_id, name, description);
	}
	else
	{
		texture_export = assetCheck(texture_id, name, description);
	}

	mTextureChecked[texture_id] = texture_export;
	
	LLStringUtil::format_map_t args;
	args["ITEM"] = name;
	
	if (!texture_export)
	{
		updateProgress(formatString(LLTrans::getString("export_asset_failed_export_check"), args));
		
		LL_DEBUGS("export") << "Texture " << texture_id << " failed export check." << LL_ENDL;
		return false;
	}

	LL_DEBUGS("export") << "Loading image texture " << texture_id << LL_ENDL;
	updateProgress(formatString(LLTrans::getString("export_loading_texture"), args));
	mRequestedTexture[texture_id].name = name;
	mRequestedTexture[texture_id].description = description;
	LLViewerFetchedTexture* image = LLViewerTextureManager::getFetchedTexture(texture_id, FTT_DEFAULT, MIPMAP_TRUE);
	image->setBoostLevel(LLViewerTexture::BOOST_MAX_LEVEL);
	image->forceToSaveRawImage(0);
	image->setLoadedCallback(FSExport::onImageLoaded, 0, TRUE, FALSE, this, &mCallbackTextureList);

	return true;
}

// static
void FSExport::onImageLoaded(BOOL success, LLViewerFetchedTexture* src_vi, LLImageRaw* src, LLImageRaw* aux_src, S32 discard_level, BOOL final, void* userdata)
{
	if(final && success)
	{
		const LLUUID& texture_id = src_vi->getID();
		LLImageJ2C* mFormattedImage = new LLImageJ2C;
		FSExportCacheReadResponder* responder = new FSExportCacheReadResponder(texture_id, mFormattedImage);
		LLAppViewer::getTextureCache()->readFromCache(texture_id, LLWorkerThread::PRIORITY_HIGH, 0, 999999, responder);
		LL_DEBUGS("export") << "Fetching " << texture_id << " from the TextureCache" << LL_ENDL;
	}
}


FSExportCacheReadResponder::FSExportCacheReadResponder(const LLUUID& id, LLImageFormatted* image) :
	mFormattedImage(image),
	mID(id)
{
	setImage(image);
}

void FSExportCacheReadResponder::setData(U8* data, S32 datasize, S32 imagesize, S32 imageformat, BOOL imagelocal)
{
	if (imageformat != IMG_CODEC_J2C)
	{
		LL_WARNS("export") << "Texture " << mID << " is not formatted as J2C." << LL_ENDL;
	}

	if (mFormattedImage.notNull())
	{	
		mFormattedImage->appendData(data, datasize);
	}
	else
	{
		mFormattedImage = LLImageFormatted::createFromType(imageformat);
		mFormattedImage->setData(data, datasize);
	}
	mImageSize = imagesize;
	mImageLocal = imagelocal;
}

void FSExportCacheReadResponder::completed(bool success)
{
	LLStringUtil::format_map_t args;
	args["TEXTURE"] = mID.asString();
	if (success && mFormattedImage.notNull() && mImageSize > 0)
	{
		LL_DEBUGS("export") << "SUCCESS getting texture " << mID << LL_ENDL;
		FSExport::getInstance()->saveFormattedImage(mFormattedImage, mID);
	}
	else
	{
		//NOTE: we can get here due to trying to fetch a static local texture
		// do nothing spiachel as importing static local texture just needs an UUID only.
		if (!success)
		{
			LL_WARNS("export") << "FAILED to get texture " << mID << LL_ENDL;
		}
		if (mFormattedImage.isNull())
		{
			LL_WARNS("export") << "FAILED: NULL texture " << mID << LL_ENDL;
		}
		FSExport::getInstance()->removeRequestedTexture(mID);
	}
}

void FSExport::removeRequestedTexture(LLUUID texture_id)
{
	if (mRequestedTexture.count(texture_id) != 0)
	{
		mRequestedTexture.erase(texture_id);
	}
}

void FSExport::saveFormattedImage(LLPointer<LLImageFormatted> mFormattedImage, LLUUID id)
{
	std::stringstream texture_str;
	texture_str.write((const char*) mFormattedImage->getData(), mFormattedImage->getDataSize());
	std::string str = texture_str.str();

	mFile["asset"][id.asString()]["name"] = mRequestedTexture[id].name;
	mFile["asset"][id.asString()]["description"] = mRequestedTexture[id].description;
	mFile["asset"][id.asString()]["type"] = LLAssetType::lookup(LLAssetType::AT_TEXTURE);
	mFile["asset"][id.asString()]["data"] = LLSD::Binary(str.begin(),str.end());

	LLStringUtil::format_map_t args;
	args["TEXTURE"] = mRequestedTexture[id].name;
	updateProgress(formatString(LLTrans::getString("export_saving_texture"), args));
	
	removeRequestedTexture(id);
}

bool FSExport::megaPrimCheck(LLUUID creator, LLViewerObject* object)
{
	F32 max_object_size = LLWorld::getInstance()->getRegionMaxPrimScale();
	LLVector3 vec = object->getScale();
	if (!(vec.mV[VX] > max_object_size || vec.mV[VY] > max_object_size || vec.mV[VZ] > max_object_size))
	{
		return false;
	}
	
	if (creator == LLUUID("7ffd02d0-12f4-48b4-9640-695708fd4ae4")) // Zwagoth Klaar
	{
		return true;
	}
	
	return false;
}

bool FSExport::defaultTextureCheck(const LLUUID asset_id)
{
	if (asset_id == LL_DEFAULT_WOOD_UUID ||
		asset_id == LL_DEFAULT_STONE_UUID ||
		asset_id == LL_DEFAULT_METAL_UUID ||
		asset_id == LL_DEFAULT_GLASS_UUID ||
		asset_id == LL_DEFAULT_FLESH_UUID ||
		asset_id == LL_DEFAULT_PLASTIC_UUID ||
		asset_id == LL_DEFAULT_RUBBER_UUID ||
		asset_id == LL_DEFAULT_LIGHT_UUID ||
		asset_id == LLUUID("5748decc-f629-461c-9a36-a35a221fe21f") ||	// UIImgWhiteUUID
		asset_id == LLUUID("8dcd4a48-2d37-4909-9f78-f7a9eb4ef903") ||	// UIImgTransparentUUID
		asset_id == LLUUID("f54a0c32-3cd1-d49a-5b4f-7b792bebc204") ||	// UIImgInvisibleUUID
		asset_id == LLUUID("6522e74d-1660-4e7f-b601-6f48c1659a77") ||	// UIImgDefaultEyesUUID
		asset_id == LLUUID("7ca39b4c-bd19-4699-aff7-f93fd03d3e7b") ||	// UIImgDefaultHairUUID
		asset_id == LLUUID("5748decc-f629-461c-9a36-a35a221fe21f")		// UIImgDefault for all clothing
	   )
	{
		LL_DEBUGS("export") << "Using system texture for " << asset_id << LL_ENDL;
		return true;
	}
	return false;
}

bool FSExport::assetCheck(LLUUID asset_id, std::string& name, std::string& description)
{
	bool exportable = false;
	LLViewerInventoryCategory::cat_array_t cats;
	LLViewerInventoryItem::item_array_t items;
	LLAssetIDMatches asset_id_matches(asset_id);
	gInventory.collectDescendentsIf(LLUUID::null,
					cats,
					items,
					LLInventoryModel::INCLUDE_TRASH,
					asset_id_matches);

	if (items.count())
	{
		// use the name of the first match
		name = items[0]->getName();
		description = items[0]->getDescription();

		for (S32 i = 0; i < items.count(); ++i)
		{
			if (!exportable)
			{
				LLPermissions perms = items[i]->getPermissions();
#if OPENSIM
				if (LLGridManager::getInstance()->isInOpenSim())
				{
					LLViewerRegion* region = gAgent.getRegion();
					if (!region) return false;
					if (region->regionSupportsExport() == LLViewerRegion::EXPORT_ALLOWED)
					{
						exportable = (perms.getMaskOwner() & PERM_EXPORT) == PERM_EXPORT;
					}
					else if (region->regionSupportsExport() == LLViewerRegion::EXPORT_DENIED)
					{
						exportable = perms.getCreator() == gAgentID;
					}
					/// TODO: Once enough grids adopt a version supporting the exports cap, get consensus
					/// on whether we should allow full perm exports anymore.
					else
					{
						exportable = (perms.getMaskBase() & PERM_ITEM_UNRESTRICTED) == PERM_ITEM_UNRESTRICTED;
					}
				}
#endif
				if (LLGridManager::getInstance()->isInSecondLife() && (perms.getCreator() == gAgentID))
				{
					exportable = true;
				}
			}
		}   
	}

	return exportable;
}

void FSExport::inventoryChanged(LLViewerObject* object, LLInventoryObject::object_list_t* inventory, S32 serial_num, void* user_data)
{
	uuid_vec_t::iterator v_iter = std::find(mInventoryRequests.begin(), mInventoryRequests.end(), object->getID());
	if (v_iter != mInventoryRequests.end())
	{
		LL_DEBUGS("export") << "Erasing inventory request " << object->getID() << LL_ENDL;
		mInventoryRequests.erase(v_iter);
	}
  
	LLSD& prim = mFile["prim"][object->getID().asString()];
	for (LLInventoryObject::object_list_t::const_iterator iter = inventory->begin(); iter != inventory->end(); ++iter)
	{
		LLInventoryItem* item = dynamic_cast<LLInventoryItem*>(iter->get());
		if (!item)
		{
			continue;
		}

		bool exportable = false;
		LLPermissions perms = item->getPermissions();
#if OPENSIM
		if (LLGridManager::getInstance()->isInOpenSim())
		{
			LLViewerRegion* region = gAgent.getRegion();
			if (region && region->regionSupportsExport() == LLViewerRegion::EXPORT_ALLOWED)
			{
				exportable = (perms.getMaskOwner() & PERM_EXPORT) == PERM_EXPORT;
			}
			else if (region && region->regionSupportsExport() == LLViewerRegion::EXPORT_DENIED)
			{
				exportable = perms.getCreator() == gAgentID;
			}
			/// TODO: Once enough grids adopt a version supporting the exports cap, get consensus
			/// on whether we should allow full perm exports anymore.
			else
			{
				exportable = ((perms.getMaskBase() & PERM_ITEM_UNRESTRICTED) == PERM_ITEM_UNRESTRICTED);
			}
		}
#endif
		if (LLGridManager::getInstance()->isInSecondLife() && (perms.getCreator() == gAgentID))
		{
			exportable = true;
		}

		if (!exportable)
		{
			//<FS:TS> Only complain if we're trying to export a non-NULL item and fail
			if (!item->getUUID().isNull())
			{
				LLStringUtil::format_map_t args;
				args["ITEM"] = item->getName();
				updateProgress(formatString(LLTrans::getString("export_asset_failed_export_check"), args));
				LL_DEBUGS("export") << "Item " << item->getName() << ", UUID " << item->getUUID() << " failed export check." << LL_ENDL;
			}
			continue;
		}

		if (item->getType() == LLAssetType::AT_NONE || item->getType() == LLAssetType::AT_OBJECT)
		{
			// currentelly not exportable
			LLStringUtil::format_map_t args;
			args["ITEM"] = item->getName();
			updateProgress(formatString(LLTrans::getString("export_item_not_exportable"), args));
			LL_DEBUGS("export") << "Skipping " << LLAssetType::lookup(item->getType()) << " item " << item->getName() << LL_ENDL;
			continue;
		}

		prim["content"].append(item->getUUID());
		mFile["inventory"][item->getUUID().asString()] = ll_create_sd_from_inventory_item(item);
		
		if (item->getAssetUUID().isNull() && item->getType() == LLAssetType::AT_NOTECARD)
		{
			// FIRE-9655
			// Blank Notecard item can have NULL asset ID.
			// Generate a new UUID and save as an empty asset.
			LLUUID asset_uuid = LLUUID::generateNewID();
			mFile["inventory"][item->getUUID().asString()]["asset_id"] = asset_uuid;
			
			mFile["asset"][asset_uuid.asString()]["name"] = item->getName();
			mFile["asset"][asset_uuid.asString()]["description"] = item->getDescription();
			mFile["asset"][asset_uuid.asString()]["type"] = LLAssetType::lookup(item->getType());

			LLNotecard nc(255); //don't need to allocate default size of 65536
			std::stringstream out_stream;
			nc.exportStream(out_stream);
			std::string out_string = out_stream.str();
			std::vector<U8> buffer(out_string.begin(), out_string.end());
			mFile["asset"][asset_uuid.asString()]["data"] = buffer;
		}
		else
		{
			LLStringUtil::format_map_t args;
			args["ITEM"] = item->getName();
			updateProgress(formatString(LLTrans::getString("export_requesting_asset"), args));
			LL_DEBUGS("export") << "Requesting asset " << item->getAssetUUID() << " for item " << item->getUUID() << LL_ENDL;
			mAssetRequests.push_back(item->getUUID());
			FSAssetResourceData* data = new FSAssetResourceData;
			data->name = item->getName();
			data->description = item->getDescription();
			data->user_data = this;
			data->uuid = item->getUUID();

			if (item->getAssetUUID().isNull() || item->getType() == LLAssetType::AT_NOTECARD || item->getType() == LLAssetType::AT_LSL_TEXT)
			{
				gAssetStorage->getInvItemAsset(object->getRegion()->getHost(),
							      gAgent.getID(),
							      gAgent.getSessionID(),
							      item->getPermissions().getOwner(),
							      object->getID(),
							      item->getUUID(),
							      item->getAssetUUID(),
							      item->getType(),
							      onLoadComplete,
							      data,
							      TRUE);
			}
			else
			{
				gAssetStorage->getAssetData(item->getAssetUUID(),
							    item->getType(),
							    onLoadComplete,
							    data,
							    TRUE);
			}
		}
	}
	
	object->removeInventoryListener(this);
}

// static
void FSExport::onLoadComplete(LLVFS* vfs, const LLUUID& asset_uuid, LLAssetType::EType type, void* user_data, S32 status, LLExtStat ext_status)
{
	FSAssetResourceData* data = (FSAssetResourceData*)user_data;
	FSExport* self = (FSExport*)data->user_data;
	LLUUID item_uuid = data->uuid;
	self->removeRequestedAsset(item_uuid);
	
	if (status != 0)
	{
		LLStringUtil::format_map_t args;
		args["ITEM"] = asset_uuid.asString();
		args["STATUS"] = llformat("%d",status);
		args["EXT_STATUS"] = llformat("%d",ext_status);
		updateProgress(formatString(LLTrans::getString("export_failed_fetch"), args));
		LL_WARNS("export") << "Problem fetching asset: " << asset_uuid << " " << status << " " << ext_status << LL_ENDL;
		delete data;
		return;
	}

	LLStringUtil::format_map_t args;
	args["ITEM"] = data->name;
	updateProgress(formatString(LLTrans::getString("export_saving_asset"), args));
	LL_DEBUGS("export") << "Saving asset " << asset_uuid.asString() << " of item " << item_uuid.asString() << LL_ENDL;
	LLVFile file(vfs, asset_uuid, type);
	S32 file_length = file.getSize();
	std::vector<U8> buffer(file_length);
	file.read(&buffer[0], file_length);
	self->mFile["asset"][asset_uuid.asString()]["name"] = data->name;
	self->mFile["asset"][asset_uuid.asString()]["description"] = data->description;
	self->mFile["asset"][asset_uuid.asString()]["type"] = LLAssetType::lookup(type);
	self->mFile["asset"][asset_uuid.asString()]["data"] = buffer;

	if (self->mFile["inventory"].has(item_uuid.asString()))
	{
		if (self->mFile["inventory"][item_uuid.asString()]["asset_id"].asUUID().isNull())
		{
			self->mFile["inventory"][item_uuid.asString()]["asset_id"] = asset_uuid;
		}
	}

	switch(type)
	{
	case LLAssetType::AT_CLOTHING:
	case LLAssetType::AT_BODYPART:
	{
		std::string asset(buffer.begin(), buffer.end());
		S32 position = asset.rfind("textures");
		boost::regex pattern("[[:xdigit:]]{8}(-[[:xdigit:]]{4}){3}-[[:xdigit:]]{12}");
		boost::sregex_iterator m1(asset.begin() + position, asset.end(), pattern);
		boost::sregex_iterator m2;
		for( ; m1 != m2; ++m1)
		{
			LL_DEBUGS("export") << "Found wearable texture " << m1->str() << LL_ENDL;
			if(LLUUID::validate(m1->str()))
			{
				self->exportTexture(LLUUID(m1->str()));
			}
			else
			{
				LL_DEBUGS("export") << "Invalid uuid: " << m1->str() << LL_ENDL;
			}
		}
	}
		break;
	case LLAssetType::AT_GESTURE:
	{
		buffer.push_back('\0');
		LLMultiGesture* gesture = new LLMultiGesture();
		LLDataPackerAsciiBuffer dp((char*)&buffer[0], file_length+1);
		if (!gesture->deserialize(dp))
		{
			LLStringUtil::format_map_t args;
			args["ITEM"] = asset_uuid.asString();
			updateProgress(formatString(LLTrans::getString("export_failed_to_load"), args));
			LL_WARNS("export") << "Unable to load gesture " << asset_uuid << LL_ENDL;
			break;
		}
		std::string name;
		std::string description;
		S32 i;
		S32 count = gesture->mSteps.size();
		for (i = 0; i < count; ++i)
		{
			LLGestureStep* step = gesture->mSteps[i];
			
			switch(step->getType())
			{
			case STEP_ANIMATION:
			{
				LLGestureStepAnimation* anim_step = (LLGestureStepAnimation*)step;
				if (!self->assetCheck(anim_step->mAnimAssetID, name, description))
				{
					LLStringUtil::format_map_t args;
					args["ITEM"] = data->name;
					updateProgress(formatString(LLTrans::getString("export_asset_failed_export_check"), args));
					LL_DEBUGS("export") << "Asset in gesture " << data->name << " failed export check." << LL_ENDL;
					break;
				}
				
				FSAssetResourceData* anim_data = new FSAssetResourceData;
				anim_data->name = anim_step->mAnimName;
				anim_data->user_data = self;
				anim_data->uuid = anim_step->mAnimAssetID;
				
				LLStringUtil::format_map_t args;
				args["ITEM"] = anim_step->mAnimAssetID.asString();
				updateProgress(formatString(LLTrans::getString("export_requesting_asset"), args));
				LL_DEBUGS("export") << "Requesting animation asset " << anim_step->mAnimAssetID.asString() << LL_ENDL;
				self->mAssetRequests.push_back(anim_step->mAnimAssetID);
				gAssetStorage->getAssetData(anim_step->mAnimAssetID,
							    LLAssetType::AT_ANIMATION,
							    onLoadComplete,
							    anim_data,
							    TRUE);
			}
				break;
			case STEP_SOUND:
			{
				LLGestureStepSound* sound_step = (LLGestureStepSound*)step;
				if (!self->assetCheck(sound_step->mSoundAssetID, name, description))
				{
					LLStringUtil::format_map_t args;
					args["ITEM"] = data->name;
					updateProgress(formatString(LLTrans::getString("export_asset_failed_export_check"), args));
					LL_DEBUGS("export") << "Asset in gesture " << data->name << " failed export check." << LL_ENDL;
					break;
				}
				
				FSAssetResourceData* sound_data = new FSAssetResourceData;
				sound_data->name = sound_step->mSoundName;
				sound_data->user_data = self;
				sound_data->uuid = sound_step->mSoundAssetID;
				
				LLStringUtil::format_map_t args;
				args["ITEM"] = sound_step->mSoundAssetID.asString();
				updateProgress(formatString(LLTrans::getString("export_requesting_asset"), args));
				LL_DEBUGS("export") << "Requesting sound asset " << sound_step->mSoundAssetID.asString() << LL_ENDL;
				self->mAssetRequests.push_back(sound_step->mSoundAssetID);
				gAssetStorage->getAssetData(sound_step->mSoundAssetID,
							    LLAssetType::AT_SOUND,
							    onLoadComplete,
							    sound_data,
							    TRUE);
			}
				break;
			default:
				break;
			}
		}
		delete gesture;
	}
		break;
	default:
		break;
	}

	delete data;
}

void FSExport::removeRequestedAsset(LLUUID asset_uuid)
{
	uuid_vec_t::iterator iter = std::find(mAssetRequests.begin(), mAssetRequests.end(), asset_uuid);
	if (iter != mAssetRequests.end())
	{
		LL_DEBUGS("export") << "Erasing asset request " << asset_uuid.asString() << LL_ENDL;
		mAssetRequests.erase(iter);
	}
}

// static
void updateProgress(const std::string message)
{
	FSFloaterObjectExport* export_floater = LLFloaterReg::findTypedInstance<FSFloaterObjectExport>("fs_export");
	if (export_floater)
		export_floater->updateProgress(message);
}

//-----------------------------------------------------
// FSFloaterObjectExport
//-----------------------------------------------------

FSFloaterObjectExport::FSFloaterObjectExport(const LLSD& key)
:	LLFloater(key),
	mOutputList(NULL)
{
}

// virtual
FSFloaterObjectExport::~FSFloaterObjectExport()
{
}

// virtual
BOOL FSFloaterObjectExport::postBuild()
{
	mOutputList = getChild<LLScrollListCtrl>("export_output");
	if (mOutputList)
		mOutputList->deleteAllItems();
	childSetAction("close_btn", boost::bind(&FSFloaterObjectExport::onCloseBtn, this));
	return TRUE;
}

void FSFloaterObjectExport::onCloseBtn()
{
	mOutputList->deleteAllItems();
	closeFloater();
}

void FSFloaterObjectExport::updateProgress(const std::string message)
{
	if (mOutputList)
	{
		LLSD row;
		row["columns"][0]["value"] = message;
		mOutputList->addElement(row);
		mOutputList->selectItemByLabel(message);
		mOutputList->scrollToShowSelected();
	}
}
