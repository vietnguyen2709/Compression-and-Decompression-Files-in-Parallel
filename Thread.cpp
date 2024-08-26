#include <pthread.h>
#include <unistd.h>
#include <cmath>
#include <iomanip>
#include <sstream>
#include <iostream>
#include <map>
#include <vector>
#include <string>
#include <queue>
#include <fstream>
#include <bitset>
#include <stdexcept>
using namespace std;

map<char, string> codes;
map<char,int> frequency;
pthread_mutex_t mutex1 = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t mutex2 = PTHREAD_MUTEX_INITIALIZER;
struct DataFromUser{
    string fileName;
    int index;
};
struct DecompressionData{
    string huffmanName;
    string fileName;
    int index;
};
struct heapNode {
    char symbol;
    unsigned freq; 
    heapNode *left, *right;
    heapNode(char symbol, unsigned freq)  
    {
        left = right = NULL;
        this->symbol = symbol;
        this->freq = freq;
    }
    heapNode(char symbol, unsigned freq, heapNode* left, heapNode* right)
    {
        this->symbol = symbol;
        this->freq = freq;
        this->left = left;
        this->right = right;
    }
     bool isLeaf() const {
        return !left && !right;
    }
};
  

struct compare {
    bool operator()(heapNode* l, heapNode* r)
    {
        return (l->freq > r->freq);
    }
};
  

void printCodes(struct heapNode* root, string str)
{  
    if (!root)
        return; 
    if (root->symbol != '$')
        cout << root->symbol << ": " << str << "\n"; 
    printCodes(root->left, str + "0");
    printCodes(root->right, str + "1");
}
  
priority_queue<heapNode*, vector<heapNode*>,
                   compare>
        heapArr; 
 
void storeCodes(struct heapNode* root, string str)
{
    if (root == NULL)
        return;
    if (root->symbol != '$')
        codes[root->symbol] = str;
    storeCodes(root->left, str + "0");
    storeCodes(root->right, str + "1");
}
 string decode_file(struct heapNode* root, string s)
{
    string ans = "";
    heapNode* curr = root;
    for (int i = 0; i < s.size(); i++) {
        if (s[i] == '0')
            curr = curr->left;
        else
            curr = curr->right;
 
        
        if ((curr->left == NULL) && (curr->right == NULL)) {
            ans += curr->symbol;
            curr = root;
        }
    }
    
    return ans ;
}
string decode_file2(const heapNode* root, const string& bitString) {
    string decodedText;
    const heapNode* curr = root;
    
    for (char bit : bitString) {
        if (bit == '0') {
            curr = curr->left;
        } else {
            curr = curr->right;
        }

        // If we've reached a leaf node, append the symbol to the result
        if (curr->left == NULL && curr->right == NULL) {
            decodedText += curr->symbol;
            curr = root; // Go back to the root for the next character
        }
    }
    
    return decodedText;
}

void HuffmanCodes(int size)
{
    struct heapNode *left, *right, *top;
    for (map<char, int>::iterator v = frequency.begin();
         v != frequency.end(); v++)
        heapArr.push(new heapNode(v->first, v->second));
    while (heapArr.size() != 1) {
        left = heapArr.top();
        heapArr.pop();
        right = heapArr.top();
        heapArr.pop();
        top = new heapNode('$',
                              left->freq + right->freq);
        top->left = left;
        top->right = right;
        heapArr.push(top);
    }
    storeCodes(heapArr.top(), "");
}

void calcFreq(string str, int n)
{
    for (int i = 0; i < str.size(); i++)
        frequency[str[i]]++;
}



vector<unsigned char> packBits(const string& bitString, int& paddingBits) {
    vector<unsigned char> byteArray;
    unsigned char currentByte = 0;
    int bitCount = 0;

    for (char bit : bitString) {
        currentByte <<= 1;  // Shift the byte left to make space for the new bit
        if (bit == '1') {
            currentByte |= 1;  // Set the least significant bit if the bit is '1'
        }
        bitCount++;

        if (bitCount == 8) {  // When we have 8 bits, store them in the byte array
            byteArray.push_back(currentByte);
            currentByte = 0;  // Reset current byte
            bitCount = 0;  // Reset bit counter
        }
    }

    // Handle the case where the last byte is not fully filled with 8 bits
    if (bitCount > 0) {
        paddingBits = 8 - bitCount;  // Calculate how many bits are missing to complete a byte
        currentByte <<= paddingBits;  // Shift remaining bits to fill the byte
        byteArray.push_back(currentByte);  // Store the last byte
    } else {
        paddingBits = 0;  // No padding needed
    }

    return byteArray;
}



string bytetobit(const vector<unsigned char>& byteArray, int paddingBits){
    string bitString = "";
    for (int i = 0; i < byteArray.size(); i++){
        bitString += bitset<8>(byteArray[i]).to_string();
    }

    // Remove padding bits from the last byte
    if (paddingBits > 0) {
        bitString.erase(bitString.end() - paddingBits, bitString.end());
    }

    return bitString;
}


void writeChunk(ofstream& outputFile, const vector<unsigned char>& byteArray, int paddingBits) {
    size_t size = byteArray.size();
    outputFile.write(reinterpret_cast<const char*>(&size), sizeof(size_t));
    
    // Write the actual byte array data
    outputFile.write(reinterpret_cast<const char*>(byteArray.data()), size);
    
    // Write the number of padding bits
    outputFile.write(reinterpret_cast<const char*>(&paddingBits), sizeof(int));
}

vector<unsigned char> readChunk(ifstream& inputFile, int& paddingBits) {
    size_t size;
    if (!inputFile.read(reinterpret_cast<char*>(&size), sizeof(size_t))) {
        if (inputFile.eof()) {
            cerr << "End of file reached." << endl;
        } else {
            cerr << "Error reading chunk size." << endl;
        }
        return {}; // Return an empty vector to indicate an error
    }

    vector<unsigned char> byteArray(size);
    if (!inputFile.read(reinterpret_cast<char*>(byteArray.data()), size)) {
        cerr << "Error reading chunk data." << endl;
        return {}; // Return an empty vector to indicate an error
    }

    // Read the padding bits info
    if (!inputFile.read(reinterpret_cast<char*>(&paddingBits), sizeof(int))) {
        cerr << "Error reading padding bits." << endl;
        return {}; // Return an empty vector to indicate an error
    }

    return byteArray;
}


void serializeTree(heapNode* root, std::ostream& os) {
    if (!root) return;

    if (root->isLeaf()) {
        os.put('0');
        os.put(root->symbol);
    } else {
        os.put('1');
        serializeTree(root->left, os);
        serializeTree(root->right, os);
    }
}

// Serialize multiple Huffman trees
void serializeMultipleTrees(const std::vector<heapNode*>& trees, std::ostream& os) {
    // Write the number of trees to the stream
    unsigned treeCount = trees.size();
    os.write(reinterpret_cast<const char*>(&treeCount), sizeof(treeCount));

    // Serialize each tree
    for (heapNode* tree : trees) {
        serializeTree(tree, os);
    }
}
heapNode* deserializeTree(std::istream& is) {
    char marker = is.get();  // Read the marker ('0' or '1')
    
    if (marker == '0') {
        char symbol = is.get();
        return new heapNode(symbol, 0);  // Leaf node
    } else if (marker == '1') {
        heapNode* leftChild = deserializeTree(is);
        heapNode* rightChild = deserializeTree(is);
        return new heapNode('\0', 0, leftChild, rightChild);  // Internal node
    }
    
    return nullptr;  // Invalid marker
}

// Deserialize multiple Huffman trees
vector<heapNode*> deserializeMultipleTrees(std::istream& is) {
    std::vector<heapNode*> trees;

    unsigned treeCount;
    is.read(reinterpret_cast<char*>(&treeCount), sizeof(treeCount));

    for (unsigned i = 0; i < treeCount; ++i) {
        trees.push_back(deserializeTree(is));
    }

    return trees;
}
void writeTree(ofstream &outputFile, heapNode* root) {
    if (root == NULL) {
        outputFile.put('#'); // Use '#' to indicate null node
        return;
    }
    // Write the symbol and its frequency
    outputFile.put(root->symbol);
    outputFile.write(reinterpret_cast<const char*>(&root->freq), sizeof(root->freq));
    
    // Recursively write left and right children
    writeTree(outputFile, root->left);
    writeTree(outputFile, root->right);
}

// Helper function to read Huffman tree from a file
heapNode* readTree(ifstream &inputFile) {
    char c = inputFile.get();
    if (c == '#') {
        return NULL; // Null node
    }

    // Read the symbol and its frequency
    char symbol = c;
    unsigned freq;
    inputFile.read(reinterpret_cast<char*>(&freq), sizeof(freq));
    
    heapNode* node = new heapNode(symbol, freq);
    
    // Recursively read left and right children
    node->left = readTree(inputFile);
    node->right = readTree(inputFile);
    
    return node;
}

// Helper function to print the Huffman tree (pre-order traversal)
void printTree(heapNode* root, string indent = "") {
    if (root == NULL) {
        cout << indent << "# (null)" << endl;
        return;
    }
    cout << indent << root->symbol << " (" << root->freq << ")" << endl;
    printTree(root->left, indent + "  ");
    printTree(root->right, indent + "  ");
}

void * ThreadFunction(void* arg_ptr){
    DataFromUser *File = (DataFromUser*) arg_ptr;
    ifstream fin;
    string line;
    fin.open(File->fileName);
    
    vector<heapNode*> roots;
    
    vector<string> encodedStrings ;
    vector<string> decodedStrings ;
    
    if(fin.is_open()){
        int count = 0;      
        while(getline(fin,line)){

            string encodedString;
            string decodedString;
            calcFreq(line,line.length());

            pthread_mutex_lock(&mutex1);

            HuffmanCodes(line.length());        
            heapNode* root = heapArr.top();   
            roots.push_back(heapArr.top());
            for (auto i : line){
            encodedString += codes[i];      
            }
            pthread_mutex_unlock(&mutex1);

            string HuffmanFile= "huffman_tree"+to_string(File->index)+".bin";
            ofstream outFile(HuffmanFile, ios::binary);
            if (outFile.is_open()) {
                vector<heapNode*> trees = roots;
                serializeMultipleTrees(trees, outFile);
                outFile.close();
            } 
            else {
                throw runtime_error("Error opening file: " + HuffmanFile);
            }          
            encodedStrings.push_back(encodedString);
            decodedStrings.push_back(decode_file(roots[count], encodedString));
            count++;          
        }
    }  
    else{
        cout << "Error opening file: " + File->fileName << endl;
        return 0;
    }
    fin.close();

    //SHOW OUTPUT FOR VIEWING IN FILE
    string outputs = "output"+to_string(File->index)+".txt";
    ofstream fout(outputs);
    for (int i = 0; i <encodedStrings.size();i++){
        fout << "Encoded Huffman data: " << endl<< encodedStrings[i];    
        fout << endl << "Decoded Huffman Data:"<< endl << decodedStrings[i] << endl << endl;
        
    }
    
    //WRITE BYTES TO THE BINARY FILE FOR COMPRESSION
    string binaryNames = "outputbin"+to_string(File->index)+".bin";
    ofstream outputbin(binaryNames,ios::binary);
    int paddingBits;
    for( int i = 0; i < encodedStrings.size();i++){
        vector<unsigned char> compressedData = packBits(encodedStrings[i],paddingBits);  
        writeChunk(outputbin,compressedData,paddingBits);
    }
    outputbin.close();

    return nullptr;
}

void* ThreadFunction2(void* arg_ptr){
    DecompressionData *File = (DecompressionData*) arg_ptr;
    
    //OPEN HUFFMAN TREE BINARY FILE FOR DECOMPRESSION
    
    ifstream inFile(File->huffmanName, ios::binary);
    vector<heapNode*> deserializedTrees;
    if(!inFile.is_open()){
        cerr << "Error opening file for reading!" << endl;
        return 0;
    }
    else{
        deserializedTrees = deserializeMultipleTrees(inFile);
        inFile.close();
    }


    //OPEN BINARY FILE TO DECOMPRESS WITH THE PROPER HUFFMAN TREE

    ifstream inputbin(File->fileName,ios::binary);

    
    ofstream decompressedFile("decompressedFile"+to_string(File->index)+".txt");
    if (!inputbin){
        cerr << "Error opening the file!" << endl;
        return 0;
    }
    int countOut = 0;
    while (inputbin.peek() != ifstream::traits_type::eof()) {
        int paddingBits;
        vector<unsigned char> byteArray = readChunk(inputbin,paddingBits);
        if (!byteArray.empty()) {
            string test = bytetobit(byteArray,paddingBits);
            string decodedstring2 =decode_file(deserializedTrees[countOut],test);
            if(decompressedFile){
                decompressedFile << decodedstring2;
            }          
            countOut++;                   
        }
    } 
    decompressedFile.close();
    inputbin.close();
    
    
    return nullptr;
}

int main(){
   
    cout <<"Do you want to compress or decompress files? (1 for compress, 2 for decompress)" << endl;
    int choice;
    while (true) {
        cin >> choice;    
        // Check if the input is invalid (cin failed)
        if (cin.fail()) {
            cin.clear();  // Clear the error flag on cin
            cin.ignore(numeric_limits<streamsize>::max(), '\n');  // Discard invalid input
            cout << "Invalid input. Please enter 1 for compression or 2 for decompression: ";
        }
        else if (choice == 1 || choice == 2) {
            break;  // Valid choice, exit loop
        }
        else {
            cout << "Please enter 1 for compression or 2 for decompression: ";
        }
    }
    if (choice == 1){
        cout <<"After compress, the compressed files will contain TWO files named as huffman_tree<number>.bin and outputbin<number>.bin" << endl;
        cout <<"huffman_tree<number>.bin is contains the key to decompress the file and outputbin<number>.bin is the compressed file that stores the data" << endl;
        cout <<"Enter the number of files to be compressed: " << endl;
        int n;
        while (true) {
            cin >> n;

            // Check if the input is invalid (cin failed) or if n is not positive
            if (cin.fail() || n <= 0) {
                cin.clear();  // Clear the error flag on cin
                cin.ignore(numeric_limits<streamsize>::max(), '\n');  // Discard invalid input
                cout << "Invalid input. Please enter a positive integer: ";
            }
            else {
                break;  // Valid input, exit loop
            }
        }
        cout <<"Enter the name of the text(.txt) files" << endl;
        DataFromUser* files = new DataFromUser [n];
        for(int i=0;i<n;i++){
            string file;
            cin>>file;
            
            files[i].fileName = file;
            files[i].index = i;
        }
        
        pthread_t tid[n];
        for (int i = 0 ; i < n; i++){            // this for loop create threads
            if (pthread_create(&tid[i], nullptr, ThreadFunction, &files[i]) != 0){
                cerr << "CANNOT CREATE THREAD" << endl;
                return 1;
            }
        }
        for(int i=0;i<n;i++){
            pthread_join(tid[i],nullptr);
        }
    }
    else if (choice == 2){
        cout <<"There are two files needed for each decompression: a binary(.bin) file and its coresponding Huffman tree(.bin) file"<<endl;
        cout <<"Enter the number of files to be decompressed: " << endl;
        int n;
        while (true) {
            cin >> n;

            // Check if the input is invalid (cin failed) or if n is not positive
            if (cin.fail() || n <= 0) {
                cin.clear();  // Clear the error flag on cin
                cin.ignore(numeric_limits<streamsize>::max(), '\n');  // Discard invalid input
                cout << "Invalid input. Please enter a positive integer: ";
            }
            else {
                break;  // Valid input, exit loop
            }
        }
        cout <<"Enter the name of the binary(.bin) files" << endl;
        DecompressionData* files = new DecompressionData [n];
        for(int i=0;i<n;i++){
            string file;
            cin>>file;
            files[i].fileName = file;
            files[i].index = i;
        }
        cout << "Enter the name of the Huffman tree(.bin) files" << endl;
        for(int i=0;i<n;i++){
            string file;
            cin>>file;
            files[i].huffmanName = file;
        }
        pthread_t tid[n];
        for (int i = 0 ; i < n; i++){            // this for loop create threads
            if (pthread_create(&tid[i], nullptr, ThreadFunction2, &files[i]) != 0){
                cerr << "CANNOT CREATE THREAD" << endl;
                return 1;
            }
        }
        for(int i=0;i<n;i++){
            pthread_join(tid[i],nullptr);
        }
    }

    return 0;
}