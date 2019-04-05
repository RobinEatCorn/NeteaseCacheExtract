#include <iostream>
#include <filesystem>
#include <string>
#include <sstream>
#include <fstream>
#include <vector>
#include <set>
#include <algorithm>

#include <curlpp/cURLpp.hpp>
#include <curlpp/Easy.hpp>
#include <curlpp/Options.hpp>

const std::filesystem::path NETEASE_DIR("./Netease_dir/");
const std::filesystem::path DESTINATION_DIR("./SMINE/netease_extract/");

const std::string ERROR_C("\033[1;31m");
const std::string FILE_C("\033[1;32m");
const std::string RESET_C("\033[0m");
const std::string MSG_C("\033[1;33m");

struct MusicLog{
	std::string ucFilename;
	std::string id;
	std::string duration;
	std::string bitrate;
	std::string musicName;
	//bool operator<(MusicLog & y){return this->id<y.id;};
	MusicLog(const MusicLog & x){
		this->ucFilename=x.ucFilename;
		this->id=x.id;
		this->duration=x.duration;
		this->bitrate=x.bitrate;
		this->musicName=x.musicName;
	};
	MusicLog(){
		this->ucFilename="";
		this->id="";
		this->duration="";
		this->bitrate="";
		this->musicName="";
	};
};

bool operator<(const MusicLog & x,const MusicLog & y){return stoull(x.id)<stoull(y.id);}

const std::vector<char> FORBIDDEN_CHARS={'/','<','>',':','"','\\','|','?','*'};

void printMusicLog(const MusicLog & lg){
	std::cout<<"musicId:"<<MSG_C<<lg.id<<RESET_C<<std::endl
			<<"duration:"<<MSG_C<<lg.duration<<RESET_C<<std::endl
			<<"bitrate:"<<MSG_C<<lg.bitrate<<RESET_C<<std::endl
			<<"musicName:"<<MSG_C<<lg.musicName<<RESET_C<<std::endl
			<<"ucFilename"<<MSG_C<<lg.ucFilename<<RESET_C<<std::endl
			<<std::endl;
}

void printSet(const std::set<MusicLog> & st){
	//------------------------DBG------------------------------
	for(auto lg:st){
		printMusicLog(lg);
	}
	//------------------------DBG------------------------------
}
std::string getValue(std::string & s,std::string sToFind){
	sToFind="\""+sToFind+"\":";
	std::size_t	stt=s.find(sToFind)+sToFind.length();
	if(stt==std::string::npos){return std::string("ERR");}
	std::size_t edd=stt;
	while('0'<=s[edd]&&s[edd]<='9')edd++;
	return s.substr(stt,edd-stt);
}

std::string getMusicName(std::string song_id){
	int lives=5;
					
	std::string music_name("");
	while(music_name=="" && lives>0){
		try{
			curlpp::Cleanup myCleanup;
			curlpp::Easy myRequest;
			
			std::list<std::string> headers;
			headers.push_back("Host: music.163.com");
			headers.push_back("Connection: keep-alive");
			//headers.push_back("Upgrade-Insecure-Requests: 1");
			headers.push_back("User-Agent: Mozilla/5.0 (Windows NT 10.0; Win64; x64) AppleWebKit/537.36 (KHTML, like Gecko) Chrome/73.0.3683.86 Safari/537.36");
			//headers.push_back("Accept: text/html,application/xhtml+xml,application/xml;q=0.9,image/webp,image/apng,*/*;q=0.8,application/signed-exchange;v=b3");
			headers.push_back("Referer: https://music.163.com/");
			//headers.push_back("Accept-Encoding: gzip, deflate, br");
			//headers.push_back("Accept-Language: zh-CN,zh;q=0.9,en;q=0.8");

			//std::fstream res("res",std::fstream::out);
			std::stringstream ss_res;
			std::string s_res;

			myRequest.setOpt(new curlpp::Options::Url("https://music.163.com/song?id="+song_id));
			myRequest.setOpt(new curlpp::Options::HttpHeader(headers));
			//myRequest.setOpt(new curlpp::Options::Verbose(true));
			myRequest.setOpt(new curlpp::Options::WriteStream(&ss_res));
			myRequest.setOpt(new curlpp::Options::Timeout(15));

			myRequest.perform();
			//res.close();
			s_res=ss_res.str();
			std::size_t stt=s_res.find("<title>"),edd=s_res.find("</title>");
			if(stt==std::string::npos){
				std::cout<<ERROR_C<<"Can't find <title>"<<RESET_C<<std::endl;continue;
			}
			if(edd==std::string::npos){
				std::cout<<ERROR_C<<"Can't find </title>"<<RESET_C<<std::endl;continue;
			}
			music_name=s_res.substr(stt+7,edd-stt-34);
			//std::cout<<"Get song's name="<<MSG_C<<music_name<<RESET_C<<std::endl;
			//std::cout<<music_name<<std::endl;
		} catch (curlpp::RuntimeError & e){
			lives--;
			std::cout<<ERROR_C<<"Error when requesting for music's name. Times left to try: "
					<<MSG_C<<lives
					<<RESET_C<<std::endl;
		}
	}
	if(lives==0){
		std::cout<<ERROR_C<<"Failed to get music's name."
					<<RESET_C<<std::endl;
		return std::string("");
	}
	return music_name;
}

void getRidOfForbiddenChars(std::string & music_name){
	for(auto & it:music_name){
		for(auto fc:FORBIDDEN_CHARS){
			if(it==fc){
				it=' ';
				break;
			}
		}	
	}
	std::cout<<"Get song's name="<<MSG_C<<music_name<<RESET_C<<std::endl;
	//Get rid of forbidden characters
}

bool betterThan(const MusicLog & x,const MusicLog & y){
	if(x.duration>y.duration){
		return true;
	} else if (x.duration==y.duration&&x.bitrate>y.bitrate){
		return true;
	}
	return false;
}


std::set<MusicLog> getFilesToConvert(std::set<MusicLog> & filesConverted){
	std::set<MusicLog> filesToConvert;
	std::filesystem::path convertLog(DESTINATION_DIR/"convert.log");
	std::ifstream convertLogf(convertLog);
	
	std::string tmp;
	unsigned long lineCnt=0;
	if(convertLogf.is_open()){
		std::cout<<"Reading "<<FILE_C<<convertLog<<RESET_C<<std::endl;
		while(std::getline(convertLogf,tmp)){
			//std::cout<<"DEBUG:tmp="<<tmp<<std::endl;
			if(tmp[0]=='#'){lineCnt++;continue;}
			MusicLog tmpLg;
			unsigned long i=0;
			while(tmp[i]!='/'&&i<tmp.length()){
				tmpLg.id+=tmp[i];
				i++;
			}
			if(i<tmp.length()-1){i++;}
			while(tmp[i]!='/'&&i<tmp.length()){
				tmpLg.duration+=tmp[i];
				i++;
			}
			if(i<tmp.length()-1){i++;}
			while(tmp[i]!='/'&&i<tmp.length()){
				tmpLg.bitrate+=tmp[i];
				i++;
			}
			if(i<tmp.length()-1){i++;}
			while(tmp[i]!='/'&&i<tmp.length()){
				tmpLg.musicName+=tmp[i];
				i++;
			}
			if(i<tmp.length()-1){i++;}

			if(tmpLg.id==""||tmpLg.duration==""||tmpLg.bitrate==""||tmpLg.musicName==""){
				std::cout<<ERROR_C<<"Bad line "<<MSG_C<<lineCnt<<RESET_C<<std::endl;
				printMusicLog(tmpLg);
				lineCnt++;
				continue;
			}
			std::string musicFilename(std::string(DESTINATION_DIR/tmpLg.musicName)+".mp3");
			std::filesystem::path musicFile(musicFilename);
			if(std::filesystem::exists(musicFile)){
				filesConverted.insert(tmpLg);
			} else {
				std::cout<<ERROR_C<<"Can't find corresponding music file: "
						<<MSG_C<<musicFilename
						<<RESET_C<<std::endl;
			}
			lineCnt++;
		}
		convertLogf.close();
	} else {
		std::cout<<MSG_C<<"convert.log not found"<<RESET_C<<std::endl;
	}
	
	for(auto &file:std::filesystem::directory_iterator(NETEASE_DIR)){
		if(file.path().extension()==".idx!"){
			MusicLog tmpLog;
			tmpLog.ucFilename=std::string(file.path().parent_path()/file.path().stem())+".uc!";
			if(std::filesystem::exists(tmpLog.ucFilename)){
				std::ifstream idxf(file.path());
				if(!idxf.is_open()){
					std::cout<<ERROR_C<<"Can't open"
							<<FILE_C<<file.path()
							<<RESET_C<<std::endl;
					continue;
				}
				std::string idxs((std::istreambuf_iterator<char>(idxf)),
							(std::istreambuf_iterator<char>()));
				idxf.close();

				std::string errWord("");
				tmpLog.id=getValue(idxs,"musicId");if(tmpLog.id=="ERR"){errWord="musicId";}
				tmpLog.duration=getValue(idxs,"duration");if(tmpLog.duration=="ERR"){errWord="duration";}
				tmpLog.bitrate=getValue(idxs,"bitrate");if(tmpLog.bitrate=="ERR"){errWord="bitrate";}
				if(errWord!=""){
					std::cout<<ERROR_C<<"Can't read Property "
							<<MSG_C<<errWord<<RESET_C<<" in "
							<<FILE_C<<file.path()
							<<RESET_C<<std::endl;
					continue;
				}
				auto logInLogSet=std::set<MusicLog>::iterator(filesConverted.find(tmpLog));
				if(logInLogSet!=filesConverted.end()){
					std::string musicNamefile=std::string(DESTINATION_DIR/logInLogSet->musicName)+".mp3";
					if(betterThan(tmpLog,*logInLogSet)){
						auto logInSet=filesToConvert.find(tmpLog);
						if(logInSet!=filesToConvert.end()){
							if(betterThan(tmpLog,*logInSet)){
								std::cout<<"Replace file "
										<<FILE_C<<musicNamefile
										<<RESET_C<<" and "
										<<FILE_C<<logInSet->ucFilename
										<<RESET_C<<" with "
										<<FILE_C<<tmpLog.ucFilename
										<<RESET_C<<std::endl;
								filesToConvert.erase(logInSet);
								filesToConvert.insert(tmpLog);
								filesConverted.erase(logInLogSet);
							}
						} else {
							filesToConvert.insert(tmpLog);
							filesConverted.erase(logInLogSet);
							std::cout<<"Replace file "
									<<FILE_C<<musicNamefile
									<<RESET_C<<" with "
									<<FILE_C<<tmpLog.ucFilename
									<<RESET_C<<std::endl;
						}
					}
				} else {
					auto logInSet=filesToConvert.find(tmpLog);
					if(logInSet!=filesToConvert.end()){
						if(betterThan(tmpLog,*logInSet)){
							std::cout<<"Replace file "
									<<FILE_C<<logInSet->ucFilename
									<<RESET_C<<" with "
									<<FILE_C<<tmpLog.ucFilename
									<<RESET_C<<std::endl;
							filesToConvert.erase(logInSet);
							filesToConvert.insert(tmpLog);
						}
					} else {
						filesToConvert.insert(tmpLog);
					}
				}
			}
		}
	}
	return filesToConvert;
}

int main(){
	std::set<MusicLog> filesConverted;
	std::set<MusicLog> filesToConvert=getFilesToConvert(filesConverted);
	unsigned long musicConvertCnt=0;

	for(auto &fInSet:filesToConvert){
		std::filesystem::directory_entry file(fInSet.ucFilename);
		std::string ori_s(file.path());
		std::string des_s(DESTINATION_DIR/(file.path().stem()));
		std::fstream ori(file.path(),std::fstream::in|std::fstream::binary);
		std::fstream des(DESTINATION_DIR/(file.path().stem()),
					std::fstream::out|std::fstream::binary);
		if(!ori.is_open()){
			std::cout<<ERROR_C<<"Can't open file "
					<<FILE_C<<ori_s
					<<RESET_C<<std::endl;
			continue;
		}
		if(!des.is_open()){
			std::cout<<ERROR_C<<"Can't open file "
					<<FILE_C<<des_s
					<<RESET_C<<std::endl;
			continue;
		}
		char c;
		while(ori.get(c)){des.put(c^0xa3);}
		ori.close();des.close();
		//xor and convert to mp3
		std::cout<<"Convert success"<<std::endl;
		
		std::string song_id(fInSet.id);
		std::cout<<"Get song's id="<<MSG_C<<song_id<<RESET_C<<std::endl;
		//get song's id

		std::string music_name=getMusicName(song_id);
		if(music_name=="")continue;
		getRidOfForbiddenChars(music_name);

		MusicLog tmpLog(fInSet);
		tmpLog.musicName=music_name;

		std::filesystem::path fileToRename(DESTINATION_DIR/(file.path().stem()));
		std::filesystem::path fileRenamedTo(DESTINATION_DIR/(music_name+".mp3"));
		if(!std::filesystem::exists(fileToRename)){
			std::cout<<ERROR_C<<"Can't find file "
					<<FILE_C<<fileToRename
					<<RESET_C<<std::endl;
			continue;
		}
		if(std::filesystem::exists(fileRenamedTo)){
			unsigned long i=1;
			while(std::filesystem::exists(fileRenamedTo=(DESTINATION_DIR/(music_name+"("+std::to_string(i)+").mp3")))){i++;}
			std::cout<<FILE_C<<DESTINATION_DIR/(music_name+".mp3")
					<<ERROR_C<<" already exists."
					<<RESET_C<<std::endl;
			tmpLog.musicName=music_name+"("+std::to_string(i)+")";
		}
		std::filesystem::rename(fileToRename,fileRenamedTo);
		std::cout<<"Rename "<<FILE_C<<fileToRename<<RESET_C
			<<"to"<<FILE_C<<fileRenamedTo<<RESET_C<<std::endl;
		//rename file

		filesConverted.insert(tmpLog);
		musicConvertCnt++;
	}	
	
	std::filesystem::path convertLogPath(DESTINATION_DIR/"convert.log");
	std::fstream convertLogf(convertLogPath,std::fstream::out);

	if(convertLogf.is_open()){
		std::cout<<"Updating "<<FILE_C<<convertLogPath<<RESET_C<<std::endl;
		for(auto & log:filesConverted){
			convertLogf<<log.id<<'/'<<log.duration<<'/'<<log.bitrate<<'/'<<log.musicName<<"\n";
		}

		convertLogf.close();
		std::cout<<"Music updated: "<<MSG_C<<musicConvertCnt<<RESET_C<<std::endl;
		std::cout<<"Total Music converted: "<<MSG_C<<filesConverted.size()<<RESET_C<<std::endl;
	} else {
		std::cout<<ERROR_C<<"Can't write convert.log"<<RESET_C<<std::endl;
	}
}
