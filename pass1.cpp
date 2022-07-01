#include "utility.cpp"
#include "tables.cpp"

using namespace std;

bool errorsFound=false;
int programLength;
string *BLocksNumToName;

string firstExecutable_Sec;

void handle_LTORG(string& litPrefix, int& lineNumberDelta,int lineNumber,int& LOCCTR, int& lastDeltaLOCCTR, int currBlockNo){
	string litAddress,litValue;
  	litPrefix = "";
  	for(auto const& it: LITTAB){
    	litAddress = it.second.addr;
    	litValue = it.second.value;
    	if(litAddress=="?"){
			lineNumber += 5;
			lineNumberDelta += 5;
			LITTAB[it.first].addr = intToStringHex(LOCCTR);
			LITTAB[it.first].blockNo = currBlockNo;
			litPrefix += "\n" + to_string(lineNumber) + "\t" + intToStringHex(LOCCTR) + "\t" + to_string(currBlockNo) + "\t" + "*" + "\t" + "="+litValue + "\t" + " " + "\t" + " ";
			if(litValue[0]=='X'){
				LOCCTR += (litValue.length() -3)/2;
				lastDeltaLOCCTR += (litValue.length() -3)/2;
			}
			else if(litValue[0]=='C'){
				LOCCTR += litValue.length() -3;
				lastDeltaLOCCTR += litValue.length() -3;
			}
    	}
  	}
}

void evaluateExpression(string expression, bool& relative,string& tempOperand,int lineNumber, ofstream& error,bool& errorsFound){
	string singleOperand="?",singleOperator="?",valueString="",valueTemp="",writeData="";
	int lastOperand=0,lastOperator=0,pairCount=0;
	char lastByte = ' ';
	bool Illegal = false;

  	for(int i=0;i<expression.length();){
    	singleOperand = "";

    	lastByte = expression[i];
		while((lastByte!='+' && lastByte!='-' && lastByte!='/' && lastByte!='*') && i<expression.length()){
			singleOperand += lastByte;
			lastByte = expression[++i];
		}

		if(SYMTAB[singleOperand].exists=='y'){
			lastOperand = SYMTAB[singleOperand].relative;
			valueTemp = to_string(stringHexToInt(SYMTAB[singleOperand].addr));
		}
		else if((singleOperand != "" || singleOperand !="?" ) && if_all_num(singleOperand)){
			lastOperand = 0;
			valueTemp = singleOperand;
		}
		else{
			writeData = "Line: "+to_string(lineNumber)+" : Can't find symbol. Found "+singleOperand;
			writeTo(error,writeData);
			Illegal = true;
			break;
		}

		if(lastOperand*lastOperator == 1){
			writeData = "Line: "+to_string(lineNumber)+" : Illegal expression";
			writeTo(error,writeData);
			errorsFound = true;
			Illegal = true;
			break;
		}
		else if((singleOperator=="-" || singleOperator=="+" || singleOperator=="?")&&lastOperand==1){
			if(singleOperator=="-"){
				pairCount--;
			}
			else{
				pairCount++;
			}
		}

    	valueString += valueTemp;

    	singleOperator= "";
		while(i<expression.length()&&(lastByte=='+'||lastByte=='-'||lastByte=='/'||lastByte=='*')){
			singleOperator += lastByte;
			lastByte = expression[++i];
		}

		if(singleOperator.length()>1){
			writeData = "Line: "+to_string(lineNumber)+" : Illegal operator in expression. Found "+singleOperator;
			writeTo(error,writeData);
			errorsFound = true;
			Illegal = true;
			break;
		}

		if(singleOperator=="*" || singleOperator == "/"){
			lastOperator = 1;
		}
		else{
			lastOperator = 0;
		}

   		valueString += singleOperator;
  	}

	if(!Illegal){
		if(pairCount==1){
			relative = 1;
			EvaluateString tempOBJ(valueString);
			tempOperand = intToStringHex(tempOBJ.getResult());
		}
		else if(pairCount==0){
			relative = 0;
			cout<<valueString<<endl;
			EvaluateString tempOBJ(valueString);
			tempOperand = intToStringHex(tempOBJ.getResult());
		}
		else{
			writeData = "Line: "+to_string(lineNumber)+" : Illegal expression";
			writeTo(error,writeData);
			errorsFound = true;
			tempOperand = "00000";
			relative = 0;
		}
	}
	else{
		tempOperand = "00000";
		errorsFound = true;
		relative = 0;
	}
}


void pass1(){

  ifstream input;
  ofstream intermediate, error;

  input.open("input.txt");
  if(!input){
    cout<<"Unable to open file: input.txt"<<endl;
    exit(1);
  }

  intermediate.open("intermediate.txt");
  if(!intermediate){
    cout<<"Unable to open file: intermediate.txt"<<endl;
    exit(1);
  }

  writeTo(intermediate,"Line\tAddress\tLabel\tOPCODE\tOPERAND\tComment");
  error.open("error.txt");
  if(!error){
    cout<<"Unable to open file: error.txt"<<endl;
    exit(1);
  }

  cout<<"Performing PASS1\n"<<endl;

  writeTo(error,"************\tPASS1\t************\n");  

  string currInst;
  string writeData, writeDataSuffix="", writeDataPrefix="";
  int index=0;

  string currBlockName = "DEFAULT";
  int currBlockNo = 0;
  int totalBlocks = 1;

  bool statusCode;
  string label, opcode, operand, comment;
  string tempOperand;

  int startAddr, LOCCTR, saveLOCCTR, lineNumber = 0, lastDeltaLOCCTR = 0, lineNumberDelta = 0;

  getline(input,currInst);
  lineNumber += 5;

  while(checkCommentLine(currInst)){
    writeData = to_string(lineNumber) + "\t" + currInst;
    writeTo(intermediate,writeData);
    getline(input,currInst);
    lineNumber += 5;
    index = 0;
  }

  readFirstNonWhiteSpace(currInst,index,statusCode,label);
  readFirstNonWhiteSpace(currInst,index,statusCode,opcode);


  if(opcode=="START"){
    readFirstNonWhiteSpace(currInst,index,statusCode,operand);
    readFirstNonWhiteSpace(currInst,index,statusCode,comment,true);
    startAddr = stringHexToInt(operand);
    LOCCTR = startAddr;
    writeData = to_string(lineNumber) + "\t" + intToStringHex(LOCCTR-lastDeltaLOCCTR) + "\t" + to_string(currBlockNo) + "\t" + label + "\t" + opcode + "\t" + operand + "\t" + comment;
    writeTo(intermediate,writeData);

    getline(input,currInst);
    lineNumber += 5;
    index = 0;
    readFirstNonWhiteSpace(currInst,index,statusCode,label);
    readFirstNonWhiteSpace(currInst,index,statusCode,opcode);
  }
  else{
    startAddr = 0;
    LOCCTR = 0;
  }
   string currentSectName="DEFAULT" ;
   int sectionCounter=0;
  while(opcode!="END"){
  		
  	while(opcode!="END" && opcode!="CSECT"){

    if(!checkCommentLine(currInst)){
      if(label!=""){
        if(SYMTAB[label].exists=='n'){
          SYMTAB[label].name = label;
          SYMTAB[label].addr = intToStringHex(LOCCTR);
          SYMTAB[label].relative = 1;
          SYMTAB[label].exists = 'y';
          SYMTAB[label].blockNo = currBlockNo;

 		  if(CSECT_TAB[currentSectName].EXTDEF_TAB[label].exists == 'y'){
 		  		CSECT_TAB[currentSectName].EXTDEF_TAB[label].addr=SYMTAB[label].addr ;
 		  }
        }
        else{
          writeData = "Line: "+to_string(lineNumber)+" : Duplicate symbol for '"+label+"'. Previously defined at "+SYMTAB[label].addr;
          writeTo(error,writeData);
          errorsFound = true;
        }
      }
      if(OPTAB[getRealOpcode(opcode)].exists=='y'){
        if(OPTAB[getRealOpcode(opcode)].format==3){
          LOCCTR += 3;
          lastDeltaLOCCTR += 3;
          if(getFlagFormat(opcode)=='+'){
            LOCCTR += 1;
            lastDeltaLOCCTR += 1;
          }
          if(getRealOpcode(opcode)=="RSUB"){
            operand = " ";
          }
          else{
            readFirstNonWhiteSpace(currInst,index,statusCode,operand);
            if(operand[operand.length()-1] == ','){
              readFirstNonWhiteSpace(currInst,index,statusCode,tempOperand);
              operand += tempOperand;
            }
          }

          if(getFlagFormat(operand)=='='){
            tempOperand = operand.substr(1,operand.length()-1);
            if(tempOperand=="*"){
              tempOperand = "X'" + intToStringHex(LOCCTR-lastDeltaLOCCTR,6) + "'";
            }
            if(LITTAB[tempOperand].exists=='n'){
              LITTAB[tempOperand].value = tempOperand;
              LITTAB[tempOperand].exists = 'y';
              LITTAB[tempOperand].addr = "?";
              LITTAB[tempOperand].blockNo = -1;
            }
          }
        }
        else if(OPTAB[getRealOpcode(opcode)].format==1){
          operand = " ";
          LOCCTR += OPTAB[getRealOpcode(opcode)].format;
          lastDeltaLOCCTR += OPTAB[getRealOpcode(opcode)].format;
        }
        else{
          LOCCTR += OPTAB[getRealOpcode(opcode)].format;
          lastDeltaLOCCTR += OPTAB[getRealOpcode(opcode)].format;
          readFirstNonWhiteSpace(currInst,index,statusCode,operand);
          if(operand[operand.length()-1] == ','){
            readFirstNonWhiteSpace(currInst,index,statusCode,tempOperand);
            operand += tempOperand;
          }
        }
      }
      else if(opcode == "EXTDEF"){

      	readFirstNonWhiteSpace(currInst,index,statusCode,operand);
		int length=operand.length() ;
		string inp="" ;
		for(int i=0;i<length;i++){
			while(operand[i]!=',' && i<length){
				inp+=operand[i] ;
				i++ ;
			}
			CSECT_TAB[currentSectName].EXTDEF_TAB[inp].name=inp ;
			CSECT_TAB[currentSectName].EXTDEF_TAB[inp].exists='y' ;
			inp="" ;
		}
      }
      else if(opcode == "EXTREF"){

      	readFirstNonWhiteSpace(currInst,index,statusCode,operand);
		int length=operand.length() ;
		string inp="" ;
		for(int i=0;i<length;i++){
			while(operand[i]!=',' && i<length){
				inp+=operand[i] ;
				i++ ;
			}
			CSECT_TAB[currentSectName].EXTREF_TAB[inp].name=inp ;
			CSECT_TAB[currentSectName].EXTREF_TAB[inp].exists='y' ;
			inp="" ;
		}
      }
      else if(opcode == "WORD"){
        readFirstNonWhiteSpace(currInst,index,statusCode,operand);
        LOCCTR += 3;
        lastDeltaLOCCTR += 3;
      }
      else if(opcode == "RESW"){
        readFirstNonWhiteSpace(currInst,index,statusCode,operand);
        LOCCTR += 3*stoi(operand);
        lastDeltaLOCCTR += 3*stoi(operand);
      }
      else if(opcode == "RESB"){
        readFirstNonWhiteSpace(currInst,index,statusCode,operand);
        LOCCTR += stoi(operand);
        lastDeltaLOCCTR += stoi(operand);
      }
      else if(opcode == "BYTE"){
        readByteOperand(currInst,index,statusCode,operand);
        if(operand[0]=='X'){
          LOCCTR += (operand.length() -3)/2;
          lastDeltaLOCCTR += (operand.length() -3)/2;
        }
        else if(operand[0]=='C'){
          LOCCTR += operand.length() -3;
          lastDeltaLOCCTR += operand.length() -3;
        }
      }
      else if(opcode=="BASE"){
        readFirstNonWhiteSpace(currInst,index,statusCode,operand);
      }
      else if(opcode=="LTORG"){
        operand = " ";
        handle_LTORG(writeDataSuffix,lineNumberDelta,lineNumber,LOCCTR,lastDeltaLOCCTR,currBlockNo);
      }
      else if(opcode=="ORG"){
        readFirstNonWhiteSpace(currInst,index,statusCode,operand);

        char lastByte = operand[operand.length()-1];
        while(lastByte=='+'||lastByte=='-'||lastByte=='/'||lastByte=='*'){
          readFirstNonWhiteSpace(currInst,index,statusCode,tempOperand);
          operand += tempOperand;
          lastByte = operand[operand.length()-1];
        }

        int tempVariable;
        tempVariable = saveLOCCTR;
        saveLOCCTR = LOCCTR;
        LOCCTR = tempVariable;

        if(SYMTAB[operand].exists=='y'){
          LOCCTR = stringHexToInt(SYMTAB[operand].addr);
        }
        else{
          bool relative;
          errorsFound = false;
          evaluateExpression(operand,relative,tempOperand,lineNumber,error,errorsFound);
          if(!errorsFound){
            LOCCTR = stringHexToInt(tempOperand);
          }
          errorsFound = false;
        }
      }
      else if(opcode=="USE"){

        readFirstNonWhiteSpace(currInst,index,statusCode,operand);
        BLOCKS[currBlockName].LOCCTR = intToStringHex(LOCCTR);

        if(BLOCKS[operand].exists=='n'){
          BLOCKS[operand].exists = 'y';
          BLOCKS[operand].name = operand;
          BLOCKS[operand].number = totalBlocks++;
          BLOCKS[operand].LOCCTR = "0";
        }

        currBlockNo = BLOCKS[operand].number;
        currBlockName = BLOCKS[operand].name;
        LOCCTR = stringHexToInt(BLOCKS[operand].LOCCTR);
      }
      else if(opcode=="EQU"){
        readFirstNonWhiteSpace(currInst,index,statusCode,operand);
        tempOperand = "";
        bool relative;

        if(operand=="*"){
          tempOperand = intToStringHex(LOCCTR-lastDeltaLOCCTR,6);
          relative = 1;
        }
        else if(if_all_num(operand)){
          tempOperand = intToStringHex(stoi(operand),6);
          relative = 0;
        }
        else{
          char lastByte = operand[operand.length()-1];
        
          while(lastByte=='+'||lastByte=='-'||lastByte=='/'||lastByte=='*'){
            readFirstNonWhiteSpace(currInst,index,statusCode,tempOperand);
            operand += tempOperand;
            lastByte = operand[operand.length()-1];
           

          }
		
          evaluateExpression(operand,relative,tempOperand,lineNumber,error,errorsFound);
        }

        SYMTAB[label].name = label;
        SYMTAB[label].addr = tempOperand;
        SYMTAB[label].relative = relative;
        SYMTAB[label].blockNo = currBlockNo;
        lastDeltaLOCCTR = LOCCTR - stringHexToInt(tempOperand);
      }
      else{
        readFirstNonWhiteSpace(currInst,index,statusCode,operand);
        writeData = "Line: "+to_string(lineNumber)+" : Invalid OPCODE. Found " + opcode;
        writeTo(error,writeData);
        errorsFound = true;
      }
      readFirstNonWhiteSpace(currInst,index,statusCode,comment,true);
      if(opcode=="EQU" && SYMTAB[label].relative == 0){
        writeData = writeDataPrefix + to_string(lineNumber) + "\t" + intToStringHex(LOCCTR-lastDeltaLOCCTR) + "\t" + " " + "\t" + label + "\t" + opcode + "\t" + operand + "\t" + comment + writeDataSuffix;
      } 

      else if(opcode=="EXTDEF" || opcode=="EXTREF"){
        writeData = writeDataPrefix + to_string(lineNumber) + "\t"+" " +"\t" +" "+"\t"+" "+ "\t" + opcode + "\t" + operand + "\t" + comment + writeDataSuffix;    
       }else if(opcode=="CSECT"){
          writeData = writeDataPrefix + to_string(lineNumber) + "\t"+intToStringHex(LOCCTR-lastDeltaLOCCTR)+"\t" +" "+"\t"+label+ "\t" + opcode + "\t" + " "+ "\t"+" " + writeDataSuffix;
    }
      else{
        writeData = writeDataPrefix + to_string(lineNumber) + "\t" + intToStringHex(LOCCTR-lastDeltaLOCCTR) + "\t" + to_string(currBlockNo) + "\t" + label + "\t" + opcode + "\t" + operand + "\t" + comment + writeDataSuffix;

      }
      writeDataPrefix = "";
      writeDataSuffix = "";
    }
    else{
      writeData = to_string(lineNumber) + "\t" + currInst;
    }
    writeTo(intermediate,writeData);

    BLOCKS[currBlockName].LOCCTR = intToStringHex(LOCCTR);
    getline(input,currInst);
    lineNumber += 5 + lineNumberDelta;
    lineNumberDelta = 0;
    index = 0;
    lastDeltaLOCCTR = 0;
    readFirstNonWhiteSpace(currInst,index,statusCode,label);
    readFirstNonWhiteSpace(currInst,index,statusCode,opcode);
  }
  	
  if(opcode!="END"){

     if(SYMTAB[label].exists=='n'){
          SYMTAB[label].name = label;
          SYMTAB[label].addr = intToStringHex(LOCCTR);
          SYMTAB[label].relative = 1;
          SYMTAB[label].exists = 'y';
          SYMTAB[label].blockNo = currBlockNo;
         }

  	CSECT_TAB[currentSectName].LOCCTR=intToStringHex(LOCCTR-lastDeltaLOCCTR,6) ;
  	CSECT_TAB[currentSectName].length=(LOCCTR-lastDeltaLOCCTR) ;
  	LOCCTR=lastDeltaLOCCTR=0;
  	currentSectName=label;
  	CSECT_TAB[currentSectName].name=currentSectName ;
  	sectionCounter++;
  	CSECT_TAB[currentSectName].sectionNo=sectionCounter ;

  	writeTo(intermediate, writeDataPrefix + to_string(lineNumber) + "\t" + intToStringHex(LOCCTR-lastDeltaLOCCTR) + "\t" + to_string(currBlockNo) + "\t" + label + "\t" + opcode);

	getline(input,currInst);
    	lineNumber += 5;

    readFirstNonWhiteSpace(currInst,index,statusCode,label);
    readFirstNonWhiteSpace(currInst,index,statusCode,opcode);
  	
  }
  else{
  	CSECT_TAB[currentSectName].LOCCTR=intToStringHex(LOCCTR-lastDeltaLOCCTR,6) ;
  	CSECT_TAB[currentSectName].length=(LOCCTR-lastDeltaLOCCTR) ;
  	
  	
  	CSECT_TAB[currentSectName].name=currentSectName ;
  	
  	CSECT_TAB[currentSectName].sectionNo=sectionCounter ;

  }
}

if(opcode=="END"){
	firstExecutable_Sec=SYMTAB[label].addr;
	SYMTAB[firstExecutable_Sec].name=label;
	SYMTAB[firstExecutable_Sec].addr=firstExecutable_Sec;
}

  readFirstNonWhiteSpace(currInst,index,statusCode,operand);
  readFirstNonWhiteSpace(currInst,index,statusCode,comment,true);

  currBlockName = "DEFAULT";
  currBlockNo = 0;
  LOCCTR = stringHexToInt(BLOCKS[currBlockName].LOCCTR);

  handle_LTORG(writeDataSuffix,lineNumberDelta,lineNumber,LOCCTR,lastDeltaLOCCTR,currBlockNo);

  writeData = to_string(lineNumber) + "\t" + intToStringHex(LOCCTR-lastDeltaLOCCTR) + "\t" + " " + "\t" + label + "\t" + opcode + "\t" + operand + "\t" + comment + writeDataSuffix;
  writeTo(intermediate,writeData);

  int LocctrArr[totalBlocks];
  BLocksNumToName = new string[totalBlocks];
  for(auto const& it: BLOCKS){
    LocctrArr[it.second.number] = stringHexToInt(it.second.LOCCTR);
    BLocksNumToName[it.second.number] = it.first;
  }

  for(int i = 1 ;i<totalBlocks;i++){
    LocctrArr[i] += LocctrArr[i-1];
  }

  for(auto const& it: BLOCKS){
    if(it.second.startAddr=="?"){
      BLOCKS[it.first].startAddr= intToStringHex(LocctrArr[it.second.number - 1]);
    }
  }

  programLength = LocctrArr[totalBlocks - 1] - startAddr;

  input.close();
  intermediate.close();
  error.close();
}
