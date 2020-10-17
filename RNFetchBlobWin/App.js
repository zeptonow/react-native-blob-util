/**
 * Sample React Native App
 * https://github.com/facebook/react-native
 *
 * @format
 * @flow strict-local
 */

import React, { useState } from 'react';
import {
  SafeAreaView,
  StyleSheet,
  ScrollView,
  View,
  Text,
  StatusBar,
  TextInput,
  Button,
  Alert,
  Picker,
} from 'react-native';

import {
  Colors,
  DebugInstructions,
  ReloadInstructions,
} from 'react-native/Libraries/NewAppScreen';

import RNFetchBlob from 'rn-fetch-blob';
 


const App: () => React$Node = () => {

// Variables ******************************************************************
  const [existsParam, setExistsParam] = useState('');
  const [lsParam, setLSParam] = useState('');

  const [cpSourceParam, setCPSourceParam] = useState('');
  const [cpDestParam, setCPDestParam] = useState('');
  
  const [unlinkParam, setUnlinkParam] = useState('');

// Methods ********************************************************************
  // exists()
  const existsCall = () => {
    RNFetchBlob.fs.exists(RNFetchBlob.fs.dirs.DocumentDir + '/' + existsParam)
    .then((result) => {
      Alert.alert('Exists: ' + result)
    })
    .catch((err) => {
      Alert.alert(err.message);
    })
  }

  const isDirCall = () => {
    RNFetchBlob.fs.exists(RNFetchBlob.fs.dirs.DocumentDir + '/' + existsParam)
    .then((result) => {
      Alert.alert('isDir: ' + result);
    })
    .catch((err) => {
      Alert.alert(err.message);
    })
  }

  // df()
  const dfCall = () => {
    RNFetchBlob.fs.df()
    .then((result) => {
      Alert.alert('Free space: ' + result.free + ' bytes\nTotal space: ' + result.total + ' bytes');
    })
    .catch((err) => {
      Alert.alert(err.message);
    })
  }

  // ls()
  const lsCall = () => {
    RNFetchBlob.fs.ls(RNFetchBlob.fs.dirs.DocumentDir + '/' + lsParam)
    .then((files) => {
        Alert.alert('Method finished: check debug console for results');
        console.log(files);
    })
  }

  // cp()
  const cpCall = () => {
    RNFetchBlob.fs.cp(RNFetchBlob.fs.dirs.DocumentDir + '/' + cpSourceParam,
    RNFetchBlob.fs.dirs.DocumentDir + '/' + cpDestParam)
    .then(
      Alert.alert('File successfully copied')
    )
    .catch((err) => {
      Alert.alert(err.message);
    });
  }

  // mv()
  const mvCall = () => {
    RNFetchBlob.fs.mv(RNFetchBlob.fs.dirs.DocumentDir + '/' + cpSourceParam,
    RNFetchBlob.fs.dirs.DocumentDir + '/' + cpDestParam)
    .then(
      Alert.alert('File successfully moved')
    )
    .catch((err) => {
      Alert.alert(err.message);
    });
  }

  // unlink()
  const unlinkCall = () => {
    RNFetchBlob.fs.unlink(RNFetchBlob.fs.dirs.DocumentDir + '/' + unlinkParam)
    .then(
      Alert.alert('file/directory successfully unlinked')
    )
    .catch((err) => {
      Alert.alert(err.message);
    })
  }

// App ************************************************************************
  return (
    <>
      <StatusBar barStyle="dark-content" />
      <SafeAreaView>
        <ScrollView
          contentInsetAdjustmentBehavior="automatic"
          style={styles.scrollView}>
          {global.HermesInternal == null ? null : (
            <View style={styles.engine}>
              <Text style={styles.footer}>Engine: Hermes</Text>
            </View>
          )}
           <Text style={styles.sectionTitle}>
            {"React Native Fetch Blob Windows Demo App"}
          </Text>
          <View style={styles.body}>
            <View style={styles.sectionContainer}>
              <View style={styles.sectionDescription}>
                <Text>
                  {"DocumentDir: " + RNFetchBlob.fs.dirs.DocumentDir + '\n'}
                  {"CacheDir: " + RNFetchBlob.fs.dirs.CacheDir + '\n'}
                  {"PictureDir: " + RNFetchBlob.fs.dirs.PictureDir + '\n'}
                  {"MusicDir: " + RNFetchBlob.fs.dirs.MusicDir + '\n'}
                  {"DownloadDir: " + RNFetchBlob.fs.dirs.DownloadDir + '\n'}
                  {"DCIMDir: " + RNFetchBlob.fs.dirs.DCIMDir + '\n'}
                  {"SDCardDir: " + RNFetchBlob.fs.dirs.SDCardDir + '\n'}
                  {"SDCardApplicationDir: " + RNFetchBlob.fs.dirs.SDCardApplicationDir + '\n'}
                  {"MainBundleDir: " + RNFetchBlob.fs.dirs.MainBundleDir + '\n'}
                  {"LibraryDir: " + RNFetchBlob.fs.dirs.LibraryDir + '\n'}
                </Text>
              </View>
            </View>
          </View>

          <View style={styles.body}>
            <View style={styles.sectionContainer}>
            <Text style={styles.sectionTitle}>
              {"exists - exists(), isDir()"}
            </Text>
              <TextInput style = {styles.input}
                placeholder = "Path"
                onChangeText={existsParam => setExistsParam(existsParam)}
                placeholderTextColor = "#9a73ef"
                autoCapitalize = "none"
              />
            <Button
              title="Check if exists"
              color="#9a73ef"
              onPress={existsCall}
            />
            <Button
              title="Check if is dir"
              color="#9a73ef"
              onPress={isDirCall}
            />
            </View>
          </View>

          <View style={styles.body}>
            <View style={styles.sectionContainer}>
            <Text style={styles.sectionTitle}>
              {"df - df()"}
            </Text>
            <Button
              title="Get free/total disk space"
              color="#9a73ef"
              onPress={dfCall}
            />
            </View>
          </View>

          <View style={styles.body}>
            <View style={styles.sectionContainer}>
            <Text style={styles.sectionTitle}>
              {"ls - ls()"}
            </Text>
            <View style={styles.sectionDescription}>
            <TextInput style = {styles.input}
              placeholder = "Directory Path"
              onChangeText={lsParam => setLSParam(lsParam)}
              placeholderTextColor = "#9a73ef"
              autoCapitalize = "none"
            />
            </View>
            <Button
              title="Get specified directory info"
              color="#9a73ef"
              onPress={lsCall}
            />
            </View>
          </View>

          <View style={styles.body}>
            <View style={styles.sectionContainer}>
            <Text style={styles.sectionTitle}>
              {"cp and mv - cp() and mv()"}
            </Text>
            <View style={styles.sectionDescription}>
            <TextInput style = {styles.input}
              placeholder = "Source File Path"
              onChangeText={cpSourceParam => setCPSourceParam(cpSourceParam)}
              placeholderTextColor = "#9a73ef"
              autoCapitalize = "none"
            />
            <TextInput style = {styles.input}
              placeholder = "Destintation File Path"
              onChangeText={cpDestParam => setCPDestParam(cpDestParam)}
              placeholderTextColor = "#9a73ef"
              autoCapitalize = "none"
            />
            </View>
            <Button
              title="Copy File to Destination"
              color="#9a73ef"
              onPress={cpCall}
            />
            <Button
              title="Move File to Destination"
              color="#9a73ef"
              onPress={mvCall}
            />
            </View>
          </View>

          <View style={styles.body}>
            <View style={styles.sectionContainer}>
            <Text style={styles.sectionTitle}>
              {"unlink - unlink()"}
            </Text>
            <View style={styles.sectionDescription}>
            <TextInput style = {styles.input}
              placeholder = "File Path"
              onChangeText={unlinkParam => setUnlinkParam(unlinkParam)}
              placeholderTextColor = "#9a73ef"
              autoCapitalize = "none"
            />
            </View>
            <Button
              title="Copy File to Destination"
              color="#9a73ef"
              onPress={unlinkCall}
            />
            </View>
          </View>  

        </ScrollView>
      </SafeAreaView>
    </>
  );
};

const styles = StyleSheet.create({
  scrollView: {
    backgroundColor: Colors.black,
  },
  engine: {
    position: 'absolute',
    right: 0,
  },
  body: {
    backgroundColor: Colors.dark,
  },
  sectionContainer: {
    marginTop: 32,
    paddingHorizontal: 24,
  },
  sectionTitle: {
    fontSize: 24,
    fontWeight: '600',
    color: Colors.white,
  },
  sectionDescription: {
    marginTop: 8,
    fontSize: 18,
    fontWeight: '400',
    color: Colors.dark,
  },
  highlight: {
    fontWeight: '700',
  },
  footer: {
    color: Colors.dark,
    fontSize: 12,
    fontWeight: '600',
    padding: 4,
    paddingRight: 12,
    textAlign: 'right',
  },
});

export default App;
