#include "pass1.cpp"

using namespace std;

ifstream intermediateFile;
ofstream errorFile,objectFile,ListingFile;

ofstream printtab ;
string writestring ;
  

bool isComment;
string label, opcode, operand, comment;
string operand1, operand2;

int lineNumber, blockNo, addr, startAddr, sectionCounter=0, program_sectionLength=0;
string objectCode, writeData, currentRecord, modificationRecord="M^", endRecord, write_R_Data, write_D_Data,currentSectName="DEFAULT";


int program_counter, base_register_value, currentTextRecordLength;
bool nobase;

string readTillTab(string data,int& index){
	string tempBuffer = "";

	while(index<data.length() && data[index] != '\t'){
		tempBuffer += data[index];
		index++;
	}
	index++;
	if(tempBuffer==" "){
		tempBuffer="-1" ;
	}
	return tempBuffer;
}
bool readIntermediateFile(ifstream& readFile,bool& isComment, int& lineNumber, int& addr, int& blockNo, string& label, string& opcode, string& operand, string& comment){
	string currInst="";
	string tempBuffer="";
	bool tempStatus;
	int index=0;
	if(!getline(readFile, currInst)){
		return false;
	}
	lineNumber = stoi(readTillTab(currInst,index));

	isComment = (currInst[index]=='.')?true:false;
	if(isComment){
		readFirstNonWhiteSpace(currInst,index,tempStatus,comment,true);
		return true;
	}
	addr = stringHexToInt(readTillTab(currInst,index));
	tempBuffer = readTillTab(currInst,index);
	if(tempBuffer == " "){
		blockNo = -1;
	}
	else{
		blockNo = stoi(tempBuffer);
	}
	label = readTillTab(currInst,index);
	if(label=="-1"){
		label=" " ;
	}
	opcode = readTillTab(currInst,index);
	if(opcode=="BYTE"){
		readByteOperand(currInst,index,tempStatus,operand);
	}
	else{
		operand = readTillTab(currInst,index);
		if(operand=="-1"){
		operand=" " ;
		}
		if(opcode=="CSECT"){
		return true ;
		}
	}
	readFirstNonWhiteSpace(currInst,index,tempStatus,comment,true);  
	return true;
	
}

string createObjectCodeFormat34(){
	string objcode;
	int halfBytes;
	halfBytes = (getFlagFormat(opcode)=='+')?5:3;

	if(getFlagFormat(operand)=='#'){
		if(operand.substr(operand.length()-2,2)==",X"){
		writeData = "Line: "+to_string(lineNumber)+" Index based addring not supported with Indirect addring";
		writeTo(errorFile,writeData);
		objcode = intToStringHex(stringHexToInt(OPTAB[getRealOpcode(opcode)].opcode)+1,2);
		objcode += (halfBytes==5)?"100000":"0000";
		return objcode;
		}

		string tempOperand = operand.substr(1,operand.length()-1);
		if(if_all_num(tempOperand)||((SYMTAB[tempOperand].exists=='y')&&(SYMTAB[tempOperand].relative==0)&&(CSECT_TAB[currentSectName].EXTREF_TAB[tempOperand].exists=='n'))){
		int immediateValue;

		if(if_all_num(tempOperand)){
			immediateValue = stoi(tempOperand);
		}
		else{
			immediateValue = stringHexToInt(SYMTAB[tempOperand].addr);
		}
		if(immediateValue>=(1<<4*halfBytes)){
			writeData = "Line: "+to_string(lineNumber)+" Immediate value exceeds format limit";
			writeTo(errorFile,writeData);
			objcode = intToStringHex(stringHexToInt(OPTAB[getRealOpcode(opcode)].opcode)+1,2);
			objcode += (halfBytes==5)?"100000":"0000";
		}
		else{
			objcode = intToStringHex(stringHexToInt(OPTAB[getRealOpcode(opcode)].opcode)+1,2);
			objcode += (halfBytes==5)?'1':'0';
			objcode += intToStringHex(immediateValue,halfBytes);
		}
		return objcode;
		}
		else if(SYMTAB[tempOperand].exists=='n'||CSECT_TAB[currentSectName].EXTREF_TAB[tempOperand].exists=='y') {
		
		if(CSECT_TAB[currentSectName].EXTREF_TAB[tempOperand].exists!='y' || halfBytes==3) { 
		writeData = "Line "+to_string(lineNumber);
		if(halfBytes==3 && CSECT_TAB[currentSectName].EXTREF_TAB[tempOperand].exists=='y'){
			writeData+= " : Invalid format for external reference " + tempOperand; 
		} else{ 
			writeData += " : Symbol doesn't exists. Found " + tempOperand;
		} 
		writeTo(errorFile,writeData);
		objcode = intToStringHex(stringHexToInt(OPTAB[getRealOpcode(opcode)].opcode)+1,2);
		objcode += (halfBytes==5)?"100000":"0000";
		return objcode;
		}
	
				if(SYMTAB[tempOperand].exists=='y'&& CSECT_TAB[currentSectName].EXTREF_TAB[tempOperand].exists=='y') {
				objcode = intToStringHex(stringHexToInt(OPTAB[getRealOpcode(opcode)].opcode)+1,2);
				objcode += "100000";

				modificationRecord += "M^" + intToStringHex(addr+1,6) + '^';
				modificationRecord += "05+";
				modificationRecord += CSECT_TAB[currentSectName].EXTREF_TAB[tempOperand].name ;
				modificationRecord += '\n';       

				return objcode ;
			}

		}
		else{
		int operandAddr = stringHexToInt(SYMTAB[tempOperand].addr) + stringHexToInt(BLOCKS[BLocksNumToName[SYMTAB[tempOperand].blockNo]].startAddr);

		if(halfBytes==5){
			objcode = intToStringHex(stringHexToInt(OPTAB[getRealOpcode(opcode)].opcode)+1,2);
			objcode += '1';
			objcode += intToStringHex(operandAddr,halfBytes);
			modificationRecord += "M^" + intToStringHex(addr+1,6) + '^';
			modificationRecord += (halfBytes==5)?"05":"03";
			modificationRecord += '\n';

			return objcode;
		}
		program_counter = addr + stringHexToInt(BLOCKS[BLocksNumToName[blockNo]].startAddr);
		program_counter += (halfBytes==5)?4:3;
		int relativeAddr = operandAddr - program_counter;
		if(relativeAddr>=(-2048) && relativeAddr<=2047){
			objcode = intToStringHex(stringHexToInt(OPTAB[getRealOpcode(opcode)].opcode)+1,2);
			objcode += '2';
			objcode += intToStringHex(relativeAddr,halfBytes);
			return objcode;
		}
		if(!nobase){
			relativeAddr = operandAddr - base_register_value;
			if(relativeAddr>=0 && relativeAddr<=4095){
			objcode = intToStringHex(stringHexToInt(OPTAB[getRealOpcode(opcode)].opcode)+1,2);
			objcode += '4';
			objcode += intToStringHex(relativeAddr,halfBytes);
			return objcode;
			}
		}

		if(operandAddr<=4095){
			objcode = intToStringHex(stringHexToInt(OPTAB[getRealOpcode(opcode)].opcode)+1,2);
			objcode += '0';
			objcode += intToStringHex(operandAddr,halfBytes);
			modificationRecord += "M^" + intToStringHex(addr+1+stringHexToInt(BLOCKS[BLocksNumToName[blockNo]].startAddr),6) + '^';
			modificationRecord += (halfBytes==5)?"05":"03";
			modificationRecord += '\n';

			return objcode;
		}
		}
	}
	else if(getFlagFormat(operand)=='@'){
		string tempOperand = operand.substr(1,operand.length()-1);
		if(tempOperand.substr(tempOperand.length()-2,2)==",X" || SYMTAB[tempOperand].exists=='n'||CSECT_TAB[currentSectName].EXTREF_TAB[tempOperand].exists=='y'){
		
	
		if(CSECT_TAB[currentSectName].EXTREF_TAB[tempOperand].exists!='y' || halfBytes==3) {
			writeData = "Line "+to_string(lineNumber);
			if(halfBytes==3 && CSECT_TAB[currentSectName].EXTREF_TAB[tempOperand].exists=='y'){
				writeData+= " : Invalid format for external reference " + tempOperand; 
			}
			else{
				writeData += " : Symbol doesn't exists.Index based addring not supported with Indirect addring ";
			}
			writeTo(errorFile,writeData);
			objcode = intToStringHex(stringHexToInt(OPTAB[getRealOpcode(opcode)].opcode)+2,2);
			objcode += (halfBytes==5)?"100000":"0000";
			return objcode;
		}

		if(SYMTAB[tempOperand].exists=='y'&& CSECT_TAB[currentSectName].EXTREF_TAB[tempOperand].exists=='y') {
			objcode = intToStringHex(stringHexToInt(OPTAB[getRealOpcode(opcode)].opcode)+2,2);
			objcode += "100000";

			modificationRecord += "M^" + intToStringHex(addr+1,6) + '^';
			modificationRecord += "05+";
			modificationRecord += CSECT_TAB[currentSectName].EXTREF_TAB[tempOperand].name ;
			modificationRecord += '\n';      

			return objcode ; 
		}
	}
		int operandAddr = stringHexToInt(SYMTAB[tempOperand].addr) + stringHexToInt(BLOCKS[BLocksNumToName[SYMTAB[tempOperand].blockNo]].startAddr);
		program_counter = addr + stringHexToInt(BLOCKS[BLocksNumToName[blockNo]].startAddr);
		program_counter += (halfBytes==5)?4:3;

		if(halfBytes==3){
			int relativeAddr = operandAddr - program_counter;
			if(relativeAddr>=(-2048) && relativeAddr<=2047){
				objcode = intToStringHex(stringHexToInt(OPTAB[getRealOpcode(opcode)].opcode)+2,2);
				objcode += '2';
				objcode += intToStringHex(relativeAddr,halfBytes);
				return objcode;
			}

			if(!nobase){
				relativeAddr = operandAddr - base_register_value;
				if(relativeAddr>=0 && relativeAddr<=4095){
				objcode = intToStringHex(stringHexToInt(OPTAB[getRealOpcode(opcode)].opcode)+2,2);
				objcode += '4';
				objcode += intToStringHex(relativeAddr,halfBytes);
				return objcode;
				}
			}

			if(operandAddr<=4095){
				objcode = intToStringHex(stringHexToInt(OPTAB[getRealOpcode(opcode)].opcode)+2,2);
				objcode += '0';
				objcode += intToStringHex(operandAddr,halfBytes);

				modificationRecord += "M^" + intToStringHex(addr+1+stringHexToInt(BLOCKS[BLocksNumToName[blockNo]].startAddr),6) + '^';
				modificationRecord += (halfBytes==5)?"05":"03";
				modificationRecord += '\n';

				return objcode;
			}
		}
		else{
			objcode = intToStringHex(stringHexToInt(OPTAB[getRealOpcode(opcode)].opcode)+2,2);
			objcode += '1';
			objcode += intToStringHex(operandAddr,halfBytes);
			modificationRecord += "M^" + intToStringHex(addr+1+stringHexToInt(BLOCKS[BLocksNumToName[blockNo]].startAddr),6) + '^';
			modificationRecord += (halfBytes==5)?"05":"03";
			modificationRecord += '\n';
			return objcode;
		}
		writeData = "Line: "+to_string(lineNumber);
		writeData += "Can't fit into program counter based or base register based addring.";
		writeTo(errorFile,writeData);
		objcode = intToStringHex(stringHexToInt(OPTAB[getRealOpcode(opcode)].opcode)+2,2);
		objcode += (halfBytes==5)?"100000":"0000";
		return objcode;
	}
	else if(getFlagFormat(operand)=='='){
		string tempOperand = operand.substr(1,operand.length()-1);
		if(tempOperand=="*"){
		tempOperand = "X'" + intToStringHex(addr,6) + "'";
		modificationRecord += "M^" + intToStringHex(stringHexToInt(LITTAB[tempOperand].addr)+stringHexToInt(BLOCKS[BLocksNumToName[LITTAB[tempOperand].blockNo]].startAddr),6) + '^';
		modificationRecord += intToStringHex(6,2);
		modificationRecord += '\n';
		}
		if(LITTAB[tempOperand].exists=='n'){
		writeData = "Line "+to_string(lineNumber)+" : Symbol doesn't exists. Found " + tempOperand;
		writeTo(errorFile,writeData);
		objcode = intToStringHex(stringHexToInt(OPTAB[getRealOpcode(opcode)].opcode)+3,2);
		objcode += (halfBytes==5)?"000":"0";
		objcode += "000";
		return objcode;
		}
		int operandAddr = stringHexToInt(LITTAB[tempOperand].addr) + stringHexToInt(BLOCKS[BLocksNumToName[LITTAB[tempOperand].blockNo]].startAddr);
		program_counter = addr + stringHexToInt(BLOCKS[BLocksNumToName[blockNo]].startAddr);
		program_counter += (halfBytes==5)?4:3;
		if(halfBytes==3){
		int relativeAddr = operandAddr - program_counter;
		if(relativeAddr>=(-2048) && relativeAddr<=2047){
			objcode = intToStringHex(stringHexToInt(OPTAB[getRealOpcode(opcode)].opcode)+3,2);
			objcode += '2';
			objcode += intToStringHex(relativeAddr,halfBytes);
			return objcode;
		}
		if(!nobase){
			relativeAddr = operandAddr - base_register_value;
			if(relativeAddr>=0 && relativeAddr<=4095){
			objcode = intToStringHex(stringHexToInt(OPTAB[getRealOpcode(opcode)].opcode)+3,2);
			objcode += '4';
			objcode += intToStringHex(relativeAddr,halfBytes);
			return objcode;
			}
		}
		if(operandAddr<=4095){
			objcode = intToStringHex(stringHexToInt(OPTAB[getRealOpcode(opcode)].opcode)+3,2);
			objcode += '0';
			objcode += intToStringHex(operandAddr,halfBytes);
			modificationRecord += "M^" + intToStringHex(addr+1+stringHexToInt(BLOCKS[BLocksNumToName[blockNo]].startAddr),6) + '^';
			modificationRecord += (halfBytes==5)?"05":"03";
			modificationRecord += '\n';
			return objcode;
		}
		}
		else{
		objcode = intToStringHex(stringHexToInt(OPTAB[getRealOpcode(opcode)].opcode)+3,2);
		objcode += '1';
		objcode += intToStringHex(operandAddr,halfBytes);
		modificationRecord += "M^" + intToStringHex(addr+1+stringHexToInt(BLOCKS[BLocksNumToName[blockNo]].startAddr),6) + '^';
		modificationRecord += (halfBytes==5)?"05":"03";
		modificationRecord += '\n';
		return objcode;
		}
		writeData = "Line: "+to_string(lineNumber);
		writeData += "Can't fit into program counter based or base register based addring.";
		writeTo(errorFile,writeData);
		objcode = intToStringHex(stringHexToInt(OPTAB[getRealOpcode(opcode)].opcode)+3,2);
		objcode += (halfBytes==5)?"100":"0";
		objcode += "000";
		return objcode;
	}
	else{
		int xbpe=0;
		string tempOperand = operand;
		if(operand.substr(operand.length()-2,2)==",X"){
		tempOperand = operand.substr(0,operand.length()-2);
		xbpe = 8;
		}
		if(SYMTAB[tempOperand].exists=='n'||CSECT_TAB[currentSectName].EXTREF_TAB[tempOperand].exists=='y') {
		if(CSECT_TAB[currentSectName].EXTREF_TAB[tempOperand].exists!='y' || halfBytes==3) {
		writeData = "Line "+to_string(lineNumber);
		if(halfBytes==3 && CSECT_TAB[currentSectName].EXTREF_TAB[tempOperand].exists=='y'){
			writeData+= " : Invalid format for external reference " + tempOperand; 
		} else{
			writeData += " : Symbol doesn't exists. Found " + tempOperand;
		}
		writeTo(errorFile,writeData);
		objcode = intToStringHex(stringHexToInt(OPTAB[getRealOpcode(opcode)].opcode)+3,2);
		objcode += (halfBytes==5)?(intToStringHex(xbpe+1,1)+"00"):intToStringHex(xbpe,1);
		objcode += "000";
		return objcode;
		}

	if(SYMTAB[tempOperand].exists=='y'&& CSECT_TAB[currentSectName].EXTREF_TAB[tempOperand].exists=='y') {
				objcode = intToStringHex(stringHexToInt(OPTAB[getRealOpcode(opcode)].opcode)+3,2);
				objcode += "100000";
				modificationRecord += "M^" + intToStringHex(addr+1,6) + '^';
				modificationRecord += "05+";
				modificationRecord += CSECT_TAB[currentSectName].EXTREF_TAB[tempOperand].name ;
				modificationRecord += '\n';       
				return objcode ;
			}
		}
	else{
		int operandAddr = stringHexToInt(SYMTAB[tempOperand].addr) + stringHexToInt(BLOCKS[BLocksNumToName[SYMTAB[tempOperand].blockNo]].startAddr);
		program_counter = addr + stringHexToInt(BLOCKS[BLocksNumToName[blockNo]].startAddr);
		program_counter += (halfBytes==5)?4:3;
		if(halfBytes==3){
		int relativeAddr = operandAddr - program_counter;
		if(relativeAddr>=(-2048) && relativeAddr<=2047){
			objcode = intToStringHex(stringHexToInt(OPTAB[getRealOpcode(opcode)].opcode)+3,2);
			objcode += intToStringHex(xbpe+2,1);
			objcode += intToStringHex(relativeAddr,halfBytes);
			return objcode;
		}
		if(!nobase){
			relativeAddr = operandAddr - base_register_value;
			if(relativeAddr>=0 && relativeAddr<=4095){
			objcode = intToStringHex(stringHexToInt(OPTAB[getRealOpcode(opcode)].opcode)+3,2);
			objcode += intToStringHex(xbpe+4,1);
			objcode += intToStringHex(relativeAddr,halfBytes);
			return objcode;
			}
		}
		if(operandAddr<=4095){
			objcode = intToStringHex(stringHexToInt(OPTAB[getRealOpcode(opcode)].opcode)+3,2);
			objcode += intToStringHex(xbpe,1);
			objcode += intToStringHex(operandAddr,halfBytes);
			modificationRecord += "M^" + intToStringHex(addr+1+stringHexToInt(BLOCKS[BLocksNumToName[blockNo]].startAddr),6) + '^';
			modificationRecord += (halfBytes==5)?"05":"03";
			modificationRecord += '\n';
			return objcode;
		}
		}
		else{
		objcode = intToStringHex(stringHexToInt(OPTAB[getRealOpcode(opcode)].opcode)+3,2);
		objcode += intToStringHex(xbpe+1,1);
		objcode += intToStringHex(operandAddr,halfBytes);
		modificationRecord += "M^" + intToStringHex(addr+1+stringHexToInt(BLOCKS[BLocksNumToName[blockNo]].startAddr),6) + '^';
		modificationRecord += (halfBytes==5)?"05":"03";
		modificationRecord += '\n';
		return objcode;
		}
		writeData = "Line: "+to_string(lineNumber);
		writeData += "Can't fit into program counter based or base register based addring.";
		writeTo(errorFile,writeData);
		objcode = intToStringHex(stringHexToInt(OPTAB[getRealOpcode(opcode)].opcode)+3,2);
		objcode += (halfBytes==5)?(intToStringHex(xbpe+1,1)+"00"):intToStringHex(xbpe,1);
		objcode += "000";
		return objcode;
	}}
	}
	void writeTextRecord(bool lastRecord=false){
	if(lastRecord){
		if(currentRecord.length()>0){
		writeData = intToStringHex(currentRecord.length()/2,2) + '^' + currentRecord;
		writeTo(objectFile,writeData);
		currentRecord = "";
		}
		return;
	}
	if(objectCode != ""){
		if(currentRecord.length()==0){
		writeData = "T^" + intToStringHex(addr+stringHexToInt(BLOCKS[BLocksNumToName[blockNo]].startAddr),6) + '^';
		writeTo(objectFile,writeData,false);
		}
		if((currentRecord + objectCode).length()>60){
		writeData = intToStringHex(currentRecord.length()/2,2) + '^' + currentRecord;
		writeTo(objectFile,writeData);
		currentRecord = "";
		writeData = "T^" + intToStringHex(addr+stringHexToInt(BLOCKS[BLocksNumToName[blockNo]].startAddr),6) + '^';
		writeTo(objectFile,writeData,false);
		}
		currentRecord += objectCode;
	}
	else{
		if(opcode=="START"||opcode=="END"||opcode=="BASE"||opcode=="NOBASE"||opcode=="LTORG"||opcode=="ORG"||opcode=="EQU"/****/||opcode=="EXTREF"||opcode=="EXTDEF"/******/){
		}
		else{
		if(currentRecord.length()>0){
			writeData = intToStringHex(currentRecord.length()/2,2) + '^' + currentRecord;
			writeTo(objectFile,writeData);
		}
		currentRecord = "";
		}
	}
}

void writeDRecord(){
    write_D_Data="D^" ;
    string temp_addr="" ;
    int length=operand.length() ;
    string inp="" ;
    for(int i=0;i<length;i++){
      while(operand[i]!=',' && i<length){
        inp+=operand[i] ;
        i++ ;
      }
      temp_addr=CSECT_TAB[currentSectName].EXTDEF_TAB[inp].addr ;
      write_D_Data+=expandString(inp,6,' ',true)+temp_addr;
      inp="" ;
    }
    writeTo(objectFile,write_D_Data);
}

void writeRRecord(){
    write_R_Data="R^" ;
    string temp_addr="" ;
    int length=operand.length() ;
    string inp="" ;
    for(int i=0;i<length;i++){
      while(operand[i]!=',' && i<length){
        inp+=operand[i] ;
        i++ ;
      }
      write_R_Data+=expandString(inp, 6, ' ', true);
      inp="" ;
    }
    writeTo(objectFile,write_R_Data);
}

void writeEndRecord(bool write=true){
  if(write){
    if(endRecord.length()>0){
      writeTo(objectFile,endRecord);
     
    }
    else{
      writeEndRecord(false);
    }
  }
  if((operand==""||operand==" ")&&currentSectName=="DEFAULT"){
    endRecord = "E^" + intToStringHex(startAddr,6);
  }else if(currentSectName!="DEFAULT"){
    endRecord = "E";
  }
  else{
    int firstExecutableAddr;
   
      firstExecutableAddr = stringHexToInt(SYMTAB[firstExecutable_Sec].addr);
    
    endRecord = "E^" + intToStringHex(firstExecutableAddr,6)+"\n";
  }
  
}

void pass2(){

	cout<<"\nPerforming PASS2\n"<<endl;
	cout<<"Writing object file to 'object.txt'"<<endl;
	cout<<"Writing listing file to 'listing.txt'"<<endl;

  string tempBuffer;
  intermediateFile.open("intermediate.txt");
  if(!intermediateFile){

    cout<<"Unable to open file: intermediate.txt"<<endl;
    exit(1);
  }
  getline(intermediateFile, tempBuffer);

  objectFile.open("object.txt");
  if(!objectFile){
    cout<<"Unable to open file: object.txt"<<endl;
    exit(1);
  }

  ListingFile.open("listing.txt");
  if(!ListingFile){
    cout<<"Unable to open file: listing.txt"<<endl;
    exit(1);
  }
  writeTo(ListingFile,"Line\tAddr\tLabel\tOPCODE\tOPERAND\tObjectCode\tComment");

  errorFile.open("error.txt",fstream::app);
  if(!errorFile){
    cout<<"Unable to open file: error.txt"<<endl;
    exit(1);
  }
  writeTo(errorFile,"\n************\tPASS2\t************\n");
  objectCode = "";
  currentTextRecordLength=0;
  currentRecord = "";
  modificationRecord = "";
  blockNo = 0;
  nobase = true;

  readIntermediateFile(intermediateFile,isComment,lineNumber,addr,blockNo,label,opcode,operand,comment);
  while(isComment){
    writeData = to_string(lineNumber) + "\t" + comment;
    writeTo(ListingFile,writeData);
    readIntermediateFile(intermediateFile,isComment,lineNumber,addr,blockNo,label,opcode,operand,comment);
  }

  if(opcode=="START"){
    startAddr = addr;
    writeData = to_string(lineNumber) + "\t" + intToStringHex(addr) + "\t" + to_string(blockNo) + "\t" + label + "\t" + opcode + "\t" + operand + "\t" + objectCode +"\t" + comment;
    writeTo(ListingFile,writeData);
  }
  else{
    label = "";
    startAddr = 0;
    addr = 0;
  }
  
  if(BLOCKS.size()>1){
    program_sectionLength = programLength ;
  }
  else{
    program_sectionLength=CSECT_TAB[currentSectName].length ;
  }
  
  writeData = "H^"+expandString(label,6,' ',true)+'^'+intToStringHex(addr,6)+'^'+intToStringHex(program_sectionLength,6);
  writeTo(objectFile,writeData);
 
  readIntermediateFile(intermediateFile,isComment,lineNumber,addr,blockNo,label,opcode,operand,comment);
  currentTextRecordLength  = 0;

  while(opcode!="END"){
      while(opcode!="END" && opcode!="CSECT"){
    if(!isComment){
      if(OPTAB[getRealOpcode(opcode)].exists=='y'){
        if(OPTAB[getRealOpcode(opcode)].format==1){
          objectCode = OPTAB[getRealOpcode(opcode)].opcode;
        }
        else if(OPTAB[getRealOpcode(opcode)].format==2){
          operand1 = operand.substr(0,operand.find(','));
          operand2 = operand.substr(operand.find(',')+1,operand.length()-operand.find(',') -1 );

          if(operand2==operand){
            if(getRealOpcode(opcode)=="SVC"){
              objectCode = OPTAB[getRealOpcode(opcode)].opcode + intToStringHex(stoi(operand1),1) + '0';
            }
            else if(REGTAB[operand1].exists=='y'){
              objectCode = OPTAB[getRealOpcode(opcode)].opcode + REGTAB[operand1].num + '0';
            }
            else{
              objectCode = getRealOpcode(opcode) + '0' + '0';
              writeData = "Line: "+to_string(lineNumber)+" Invalid Register name";
              writeTo(errorFile,writeData);
            }
          }
          else{
            if(REGTAB[operand1].exists=='n'){
              objectCode = OPTAB[getRealOpcode(opcode)].opcode + "00";
              writeData = "Line: "+to_string(lineNumber)+" Invalid Register name";
              writeTo(errorFile,writeData);
            }
            else if(getRealOpcode(opcode)=="SHIFTR" || getRealOpcode(opcode)=="SHIFTL"){
              objectCode = OPTAB[getRealOpcode(opcode)].opcode + REGTAB[operand1].num + intToStringHex(stoi(operand2),1);
            }
            else if(REGTAB[operand2].exists=='n'){
              objectCode = OPTAB[getRealOpcode(opcode)].opcode + "00";
              writeData = "Line: "+to_string(lineNumber)+" Invalid Register name";
              writeTo(errorFile,writeData);
            }
            else{
              objectCode = OPTAB[getRealOpcode(opcode)].opcode + REGTAB[operand1].num + REGTAB[operand2].num;
            }
          }
        }
        else if(OPTAB[getRealOpcode(opcode)].format==3){
          if(getRealOpcode(opcode)=="RSUB"){
            objectCode = OPTAB[getRealOpcode(opcode)].opcode;
            objectCode += (getFlagFormat(opcode)=='+')?"000000":"0000";
          }
          else{
            objectCode = createObjectCodeFormat34();
          }
        }
      }
      else if(opcode=="BYTE"){
        if(operand[0]=='X'){
          objectCode = operand.substr(2,operand.length()-3);
        }
        else if(operand[0]=='C'){
          objectCode = stringToHexString(operand.substr(2,operand.length()-3));
        }
      }
      else if(label=="*"){
        if(opcode[1]=='C'){
          objectCode = stringToHexString(opcode.substr(3,opcode.length()-4));
        }
        else if(opcode[1]=='X'){
          objectCode = opcode.substr(3,opcode.length()-4);
        }
      }
      else if(opcode=="WORD"){
        objectCode = intToStringHex(stoi(operand),6);
      }
      else if(opcode=="BASE"){
        if(SYMTAB[operand].exists=='y'){
          base_register_value = stringHexToInt(SYMTAB[operand].addr) + stringHexToInt(BLOCKS[BLocksNumToName[SYMTAB[operand].blockNo]].startAddr);
          nobase = false;
        }
        else{
          writeData = "Line "+to_string(lineNumber)+" : Symbol doesn't exists. Found " + operand;
          writeTo(errorFile,writeData);
        }
        objectCode = "";
      }
      else if(opcode=="NOBASE"){
        if(nobase){
          writeData = "Line "+to_string(lineNumber)+": Assembler wasn't using base addring";
          writeTo(errorFile,writeData);
        }
        else{
          nobase = true;
        }
        objectCode = "";
      }
      else{
        objectCode = "";
      }
      
      writeTextRecord();

      if(blockNo==-1 && addr!=-1){
        writeData = to_string(lineNumber) + "\t" + intToStringHex(addr) + "\t" + " " + "\t" + label + "\t" + opcode + "\t" + operand + "\t" + objectCode +"\t" + comment;
      }
      else if(addr==-1){
        if(opcode=="EXTDEF"){
          writeDRecord() ;
        } else if(opcode=="EXTREF"){
          writeRRecord() ;
        }
        writeData = to_string(lineNumber) + "\t" + " " + "\t" + " " + "\t" + label + "\t" + opcode + "\t" + operand + "\t" + objectCode +"\t" + comment;
      } 
     
      else{ writeData = to_string(lineNumber) + "\t" + intToStringHex(addr) + "\t" + to_string(blockNo) + "\t" + label + "\t" + opcode + "\t" + operand + "\t" + objectCode +"\t" + comment;
      }
    }
    else{
      writeData = to_string(lineNumber) + "\t" + comment;
    }
    writeTo(ListingFile,writeData);
    readIntermediateFile(intermediateFile,isComment,lineNumber,addr,blockNo,label,opcode,operand,comment);
    objectCode = "";
  }
  writeTextRecord();

  writeEndRecord(false);
  
   if(opcode=="CSECT"&&!isComment){
        writeData = to_string(lineNumber) + "\t" + intToStringHex(addr) + "\t" + to_string(blockNo) + "\t" + label + "\t" + opcode + "\t" + " " + "\t" + objectCode +"\t"+" ";
      }else if(!isComment){
  writeData = to_string(lineNumber) + "\t" + intToStringHex(addr) + "\t" + " " + "\t" + label + "\t" + opcode + "\t" + operand + "\t" + "" +"\t" + comment;
}else{
  writeData = to_string(lineNumber) + "\t" + comment;
 }
  writeTo(ListingFile,writeData);

if(opcode!="CSECT"){
  while(readIntermediateFile(intermediateFile,isComment,lineNumber,addr,blockNo,label,opcode,operand,comment)){
    if(label=="*"){
      if(opcode[1]=='C'){
        objectCode = stringToHexString(opcode.substr(3,opcode.length()-4));
      }
      else if(opcode[1]=='X'){
        objectCode = opcode.substr(3,opcode.length()-4);
      }
      writeTextRecord();
    }
    writeData = to_string(lineNumber) + "\t" + intToStringHex(addr) + "\t" + to_string(blockNo) + label + "\t" + opcode + "\t" + operand + "\t" + objectCode +"\t" + comment;
    writeTo(ListingFile,writeData);
  }
}   
  
writeTextRecord(true);
if(!isComment){
  
  writeTo(objectFile,modificationRecord,false);
  writeEndRecord(true);
  currentSectName=label;
  modificationRecord="";
}
if(!isComment&&opcode!="END"){
writeData = "\n************object program for "+ label+" ************";
  writeTo(objectFile,writeData);
  


writeData = "\nH^"+expandString(label,6,' ',true)+'^'+intToStringHex(addr,6)+'^'+intToStringHex(CSECT_TAB[label].length,6);
  writeTo(objectFile,writeData);
  }
  readIntermediateFile(intermediateFile,isComment,lineNumber,addr,blockNo,label,opcode,operand,comment);
    objectCode = "";

}
}

int main(){

    load_OPTAB();
	start_BLOCKS();
	
	pass1();

	cout<<"Writing SYMBOL TABLE"<<endl;
	printtab.open("tables.txt") ;
	writeTo(printtab,"************\tSYMBOL TABLE\t************\n") ;
	for (auto const& it: SYMTAB) { 
		writestring+=it.first+":-\t"+ "name:"+it.second.name+"\t|"+ "addr:"+it.second.addr+"\t|"+ "relative:"+intToStringHex(it.second.relative)+" \n" ;
	} 
	writeTo(printtab,writestring) ;

	writestring="" ;
		cout<<"Writing LITERAL TABLE"<<endl;
	
	writeTo(printtab,"************\tLITERAL TABLE\t************\n") ;
		for (auto const& it: LITTAB) { 
			writestring+=it.first+":-\t"+ "value:"+it.second.value+"\t|"+ "addr:"+it.second.addr+" \n" ;
		} 
		writeTo(printtab,writestring) ;

	writestring="" ;
		cout<<"Writing EXTREF TABLE"<<endl;

	writeTo(printtab,"************\tEXTREF TABLE\t************\n") ;
		for (auto const& it0: CSECT_TAB) { 
		for (auto const& it: it0.second.EXTREF_TAB) 
			writestring+=it.first+":-\t"+ "name:"+it.second.name+"\t|"+it0.second.name+" \n" ;
		
		} 
		writeTo(printtab,writestring) ;


	writestring="" ;
		cout<<"Writing EXTDEF TABLE"<<endl;
	
	writeTo(printtab,"************\tEXTDEF TABLE\t************\n") ;
		for (auto const& it0: CSECT_TAB) {
		for (auto const& it: it0.second.EXTDEF_TAB) {
			if(it.second.name!="undefined")
			writestring+=it.first+":-\t"+ "name:"+it.second.name+"\t|"+ "addr:"+it.second.addr+"\t|"+" \n" ;
		}
		}
		
		writeTo(printtab,writestring) ;


	pass2();



}
